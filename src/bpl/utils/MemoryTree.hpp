////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#ifndef __BPL_UTILS_MEMORY_TREE__
#define __BPL_UTILS_MEMORY_TREE__

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <limits>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

#ifdef DPU
#define DEBUG(a...)  //if (me()==0)  {  printf(a); }
#else
#define DEBUG(a...)  //printf(a)
#endif

////////////////////////////////////////////////////////////////////////////////

/** Manage memory addresses as a tree.
 * Each node of the tree holds 2^NBITEMS_PER_BLOCK_LOG2 children.
 *
 * The size of this class is determined by the MAX_MEMORY_LOG2 parameter, ie
 * the sizeof will be of order 2^MAX_MEMORY_LOG2.
 *
 * According to the choice of NBITEMS_PER_BLOCK_LOG2 and MAX_MEMORY_LOG2, the maximum
 * number of items for this structure is computed (attribute NBITEMS_MAX)
 */
template <typename ALLOCATOR, int NBITEMS_PER_BLOCK_LOG2, int MAX_MEMORY_LOG2>
class MemoryTree
{
public:

    using address_t = typename ALLOCATOR::address_t;

    using size_type  = uint32_t;
    using depth_t    = int8_t;

    // we estimate the max depth of the tree according to the max memory we can use.
    // the biggest memory comes from the stack (-> address_t items_ [NBITEMS_PER_BLOCK * LEVEL_MAX];)
    static constexpr depth_t LEVEL_MAX = 2 + (1ULL << (MAX_MEMORY_LOG2 - NBITEMS_PER_BLOCK_LOG2) ) / sizeof(address_t);

    static constexpr int NBITEMS_PER_BLOCK      = 1ULL << NBITEMS_PER_BLOCK_LOG2;
    static constexpr int NBITEMS_PER_BLOCK_MASK = (NBITEMS_PER_BLOCK-1);

    static_assert (NBITEMS_PER_BLOCK_LOG2 >= 1);
    static_assert (NBITEMS_PER_BLOCK_LOG2 < MAX_MEMORY_LOG2);
    static_assert (LEVEL_MAX >= 2);

    static constexpr u_int64_t NBITEMS_MAX =  1ULL << (LEVEL_MAX*NBITEMS_PER_BLOCK_LOG2);

    MemoryTree()
    {
        // we reset the counts for each possible depth.
        for (size_t i=0; i<sizeof(countsPerDepth_)/sizeof(countsPerDepth_[0]); i++)  { countsPerDepth_[i] = 0; }
    }

    MemoryTree (const MemoryTree&  other) = delete;
    MemoryTree (      MemoryTree&& other) = default;

    MemoryTree& operator= (const MemoryTree&  other) = delete;
    MemoryTree& operator= (      MemoryTree&& other) = default;

    ~MemoryTree()
    {
        DEBUG ("[MemoryTree::~MemoryTree]  #stack=%ld  depth=%d \n", stack_.size(), depth());

        if (ALLOCATOR::is_freeable == true)
        {
            DFS ([&] (depth_t depth, address_t value)
            {
                // we deallocate each non-leaf node through a DFS traversal.
                if (depth>0)
                {
                    DEBUG ("[MemoryTree::~MemoryTree]  depth=%d  value=0x%lx\n", depth, value);
                    ALLOCATOR::free (value);
                }
            });
        }
    }

    /** Insert a new address as a leaf of the tree.
     * When the current block is full, the tree is re-organized.
     * After an insertion, the tree can be in a "dirty" state and a flush is potentially processed by other methods.
     */
    auto insert (address_t a)
    {
        DEBUG("[MemoryTree::insert]  a=0x%lx\n", a);

        nbItems_ ++;

        // we insert the item into the stack.
        stack_.insert (a);

        // we increase the number of items for depth 0 (ie. leaves)
        countsPerDepth_[0] ++;

        // We may have to flush the content of the stack.
        flush_full_depths ();

        // we set the tree as "dirty" -> other methods could need to do a 'flush'
        isDirty_ = true;

        return a;
    }

    /** Access to a specific item given its index.
     * We do a traversal of the tree from the root to the target leaf, so we need to ask in memory the children of a node
     * but we can avoid some memory accesses in a few cases.
     */
    address_t operator[] (size_t idx)
    {
        address_t result { address_t() };

        // a flush may be required
        flush();

        // The required block may be already there
        if (idx==previousIdx_)
        {
            size_t digit = idx & NBITEMS_PER_BLOCK_MASK;

            result = * (stack_.tail () + digit) ;
        }
        else
        {
            // we assume here that the tree has been "flushed", so the stack should contain only one item
            result = * stack_.get (0);

            // accessing the required leaf consists in traversing the tree like we enumerate the digits of a number in a given basis.
            for (depth_t power=depth()-1; power>=0; power--)
            {
                size_t digit = (idx >> (power*NBITEMS_PER_BLOCK_LOG2)) & NBITEMS_PER_BLOCK_MASK;

                // DEBUG ("operator[]  idx=%ld  power=%ld  digit=%ld  #stack=%ld \n", idx, power, digit, stack_.idx_);

                // we read a block from memory and put it at the end of the stack.
                // note: we use the inner buffer of the stack here, ie. we don't use the 'insert' method
                // -> we can do that because we have had a supplementary level during the stack declaration
                // which allows to address NBITEMS_PER_BLOCK items
                // Note also that we want to go directly to a specific leaf, so we don't need to keep information
                // of intermediate nodes.
                address_t* ptr = ALLOCATOR::read (result , stack_.tail(), sizeof(result)*NBITEMS_PER_BLOCK);

                // we update the result.
                result = *(ptr+digit);
            }

            // we remember the required idx for potentially optimize lookup for next call to operator[]
            previousIdx_ = idx;
        }

        return result;
    }

    /** "ALMOST" Depth First Search of the tree.
     *  WARNING!!! this is not a "true" DFS, but the important fact is that leaves will
     *  be iterated in the same order than the one used for their insertion into the tree.
     *  The provided functor is called for each node. */
    template<typename FCT>  void DFS (FCT fct)
    {
        flush();
        iterate ([&] (depth_t depth, address_t value)  {  DFS_iter<false> (fct, depth, value);  });
    }

    /** Iteration of the leaves through a functor. */
    template<typename FCT>  void leaves (FCT fct)
    {
        flush();

        if (maxDepth_==0)
        {
            // Special case: only one root node in the tree -> we call directly the functor on it
            fct (stack_.top());
        }
        else
        {
            for (auto value : *this)  {  fct (value);  }
        }
    }

    // Iterator for the leaves of the tree.
    // WARNING ! since we try to minimize memory usage, we reuse the stack of the MemoryTree itself.
    struct leaves_iterator
    {
        using tree_t = MemoryTree<ALLOCATOR,NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2>;

        void swap (address_t& a, address_t& b)  {  address_t c(a); a=b; b=c; };
        address_t set_depth   (address_t node, depth_t d)  { return node | address_t(d) << 8*(sizeof(address_t)-sizeof(depth_t));  };
        depth_t   get_depth   (address_t node)  { return  depth_t(node >> 8*(sizeof(address_t)-sizeof(depth_t))); };
        address_t get_address (address_t node)  { return  node & ((address_t(1) << 8*(sizeof(address_t)-sizeof(depth_t)))-1); };

        //leaves_iterator() = delete;

        leaves_iterator (tree_t& ref, bool begin) : ref_(ref), previousDepth(ref.maxDepth_), begin_(begin)
        {
            if (begin_)
            {
                stackIdx_ = ref_.stack_.idx_;

                // For the 'begin' iterator, we put the root node into the queue.
                ref_.stack_.insert (set_depth(ref_.stack_.top(),previousDepth));

                if (previousDepth == 0)
                {
                    node  = get_address (ref_.stack_.top());
                    depth = get_depth   (ref_.stack_.top());
                }
                else
                {
                    this->operator++();
                }
            }
        }

        leaves_iterator (const leaves_iterator& other) : ref_(other.ref_)
        {
            if (this != &other)
            {
                node          = other.node;
                depth         = other.depth;
                previousDepth = other.previousDepth;
                std::swap (begin_, other.begin_);
            }
        }

        ~leaves_iterator ()
        {
            if (begin_)  
            {  
            	ref_.stack_.idx_ = 1; //stackIdx_;  
            }
        }

        bool operator!= (const leaves_iterator& other)  {  return ref_.stack_.size() > 1;  }

        address_t operator* ()  {  return node;  }

        leaves_iterator operator+ (size_t d) const
        {
            leaves_iterator result  = *this;

            for (size_t i=0; i<d; i++)  { ++result; }

            return result;
        }

        leaves_iterator& operator+= (size_t d)
        {
            for (size_t i=0; i<d; i++)  { ++(*this); }
            return *this;
        }

        auto& operator++ ()
        {
            while (ref_.stack_.size() > 1)
            {
                // We get the top of the stack and remove it from the stack.
                address_t head = ref_.stack_.top();

                // We get the actual address and depth from it.
                node  = get_address (head);
                depth = get_depth   (head);

                if (node==address_t())
                {
                    ref_.stack_.pop();
                    continue;
                }

                // We check whether we have finished all children.
                bool areChildrenDone = depth == (previousDepth+1);

                if (depth > 0)
                {
                    if (not areChildrenDone)
                    {
                        // We insert all the children into the stack.
                        address_t* block = ALLOCATOR::read (node, ref_.stack_.tail(), NBITEMS_PER_BLOCK*sizeof(address_t));

                        ref_.stack_.shift (NBITEMS_PER_BLOCK);

                        // If we want insertion order, we have to revert the order of the retrieved data.
                        // If we want reverse order, we don't have to do anything.
                        // NOTE: NBITEMS_PER_BLOCK must be power of 2 to make 'swap' efficient.
                        for (size_t k=0; k<NBITEMS_PER_BLOCK/2; k++)  {  swap (block[k], block[NBITEMS_PER_BLOCK-k-1]);  }

                        // And into the depth stack as well with a lesser depth 'd-1' than the current node.
                        for (size_t k=0; k<NBITEMS_PER_BLOCK; k++)
                        {
                            // We associate the depth to the address.
                            block[k] = set_depth (block[k], depth-1);
                        }
                    }
                    else
                    {
                        // We memorize the current depth for the next iteration.
                        previousDepth = depth;

                        // This node has been processed, we remove it from the stack.
                        ref_.stack_.pop();
                    }
                }

                else  // NOT if (depth > 0)
                {
                    // We memorize the current depth for the next iteration.
                    previousDepth = depth;

                    // This node has been processed, we remove it from the stack.
                    ref_.stack_.pop();

                    // In case of a (non null) leaf, we break the while loop and wait for a call to operator*.
                    // (remember that we can have null leaf in order to complete a block of size NBITEMS_PER_BLOCK)
                    break;
                }
            }

            return *this;
        }

        tree_t& ref_;
        address_t node  = address_t();
        depth_t   depth = 0;
        depth_t   previousDepth = 0;
        mutable bool      begin_ = false;
        size_t stackIdx_ = 0;
    };

    leaves_iterator begin()  { flush();  return leaves_iterator(*this, true ); }
    leaves_iterator end  ()  {           return leaves_iterator(*this, false); }

    /** Number of nodes in the tree.
     * \return Nodes number. */
    size_type size()  const { return nbItems_; }

    /** */
    size_type max_size () const { return NBITEMS_MAX; }

    /** Depth of the tree.
     * \return the depth. */
    depth_t depth() const { return maxDepth_ ; }

    /** For debug purpose.
     */
    void debug(const char* txt="tree") const
    {
        DEBUG ("%s:  ", txt);
        DEBUG ("  LEVEL_MAX=%d  NBITEMS_PER_BLOCK=%d  sizeof=%d  #items=%4d  depth=%2d  #stack=%2d  stackmax=%d  isDirty_=%d  top=0x%lx  counts:  ",
            LEVEL_MAX, NBITEMS_PER_BLOCK, sizeof(*this), size(), depth(), stack_.size(),
            sizeof(stack_.items_)/sizeof(stack_.items_[0]), isDirty_,
            stack_.size()>0 ? stack_.top() : -1

        );
        for (depth_t i=0; i<=maxDepth_; i++)  { DEBUG ("(%d,%2d)  ", i, countsPerDepth_[i]); }
        DEBUG ("\n");
    }

//private:

    void flush ()
    {
        if (isDirty_)
        {
            flush ([&](depth_t depth) { return depth < maxDepth_; });

            // we also merge the max depth if needed.
            // After that, the stack should hold only one item of depth d with countsPerDepth_[d]==1
            size_t nbItems = countsPerDepth_[maxDepth_];

            // merge required only if more than one item.
            if (nbItems > 1)
            {
                int pending = merge (maxDepth_, nbItems);

                if (pending>0)
                {
                    merge (maxDepth_-1, countsPerDepth_[maxDepth_-1]);
                    merge (maxDepth_-0, countsPerDepth_[maxDepth_-0]);
                }
            }

            isDirty_ = false;
        }
    }

    void flush_full_depths ()
    {
        for (depth_t depth=0; (countsPerDepth_[depth] == NBITEMS_PER_BLOCK) and (depth < LEVEL_MAX-1); depth++)
        {
            merge (depth, NBITEMS_PER_BLOCK);
        }
    }

    template<typename CHECK>
    void flush (CHECK check)
    {
        // we update the counts for the different depths, starting by the leaves (ie. depth==0)
        for (depth_t depth=0; check(depth) and (depth < LEVEL_MAX-1); depth++)
        {
            size_t nbItems = countsPerDepth_[depth];

            // we may dump the current block to memory
            if (nbItems > 0)  {  merge (depth, nbItems);  }
        }
    }

    /** Allocate some memory and copy a block into it. The tree structure is also updated.
     */
    int merge (depth_t depth, size_t nbItems)
    {
        DEBUG ("[MemoryTree::merge]  depth: %d  nbItems: %d   maxDepth_: %d \n", depth, nbItems, maxDepth_);

        // we add empty values to the end of the stack in order to have a complete block
        for (size_t i=nbItems ; i < NBITEMS_PER_BLOCK; i++)  {  stack_.insert (address_t()); }

        // we dump the block of addresses for the current depth into memory and get the address of it.
        address_t b = ALLOCATOR::write (stack_.get(-NBITEMS_PER_BLOCK), NBITEMS_PER_BLOCK*sizeof(address_t));

        // we pop this block of addresses from the stack
        stack_.pop (NBITEMS_PER_BLOCK);

        // we insert the new block.
        stack_.insert (b);

        // we update the counts for the current level (goes back to 0) and increase the next one.
        countsPerDepth_[depth+0] -= std::min(NBITEMS_PER_BLOCK,int(nbItems));
        countsPerDepth_[depth+1] ++;

        if (maxDepth_ < depth+1)  { maxDepth_ = depth+1; }

        return countsPerDepth_[depth+0];
    }

    ////////////////////////////////////////////////////////////////////////////////
    struct stack
    {
        void insert (address_t a)
        {
            items_ [idx_] = a;
            idx_ ++;
        }

        void pop (size_t nb=1)
        {
            idx_ -= nb;
        }

        void shift (size_t nb)
        {
            idx_ += nb;
        }

        size_t size() const { return idx_; }

        /** same convention as python for index i, i.e. -1 corresponds to the last item. */
        /** Don't use ssize_t for the input parameter => LEAD TO WRONG RESULTS... */
        address_t* get (int32_t i)  { return & items_[i>=0 ? i : idx_ + i ]; }

        // top
        address_t top()  const { return  items_[idx_-1]; }

        // end of the stack.
        address_t* tail ()  { return & items_[idx_]; }

        uint32_t  idx_ = 0;
        address_t items_ [NBITEMS_PER_BLOCK * LEVEL_MAX + 1];  // we need +1 when block aggregation occurs (same block seen twice at two depths)
    };

    // iterate each value of the stack with its associated depth
    template<typename FCT>  void iterate (FCT fct)
    {
        if (stack_.idx_==0)  { return; }  // needed if tree is empty

        depth_t depth = maxDepth_;

        size_t i=0;
        for (size_t count = countsPerDepth_ [depth]; depth >= 0; ++i)
        {
            // we notify the depth and the item of the stack.
            fct (depth, stack_.items_[i]);

            if (--count==0)  // go to next depth
            {
                for (--depth; depth>=0; --depth)
                {
                    // we set the new count for the new level.
                    count = countsPerDepth_ [depth];

                    // in case the new level has some items, we must break and not let the --depth of the for loop
                    if (count>0) { break; }
                }
            }
        }
    }

    template<typename FCT>  void DFS_rec (FCT fct, depth_t depth, address_t value)
    {
        // Note: we have to check that the value is valid since we may have blocks filled with empty
        // values in order to have always the same block size.
        if (depth > 0 and value != address_t())
        {
            // we retrieve the block for the current value.
            // Note: we use the stack to hold the data to be read, ie we will put the incoming data to the current tail of the stack.
            address_t* block = ALLOCATOR::read (value, stack_.tail(), NBITEMS_PER_BLOCK*sizeof(address_t));

            // we must tell to the stack that we just made N "insertions"
            stack_.shift (NBITEMS_PER_BLOCK);

            for (size_t i=0; i<NBITEMS_PER_BLOCK; i++)
            {
                DFS_rec (fct, depth-1, block[i]);
            }

            // the items of the block have been processed, we remove them from the stack.
            stack_.pop (NBITEMS_PER_BLOCK);
        }

        // we call the functor (DFS so we traverse children first).
        fct (depth, value);
    };

    template<bool REVERSED, typename FCT>  void DFS_iter (FCT fct, depth_t depth, address_t node)
    {
        // We first need to flush the tree -> reorganize the tree in order to have only the root node in the stack.
        flush();

        auto swap = [] (address_t& a, address_t& b)  {  address_t c(a); a=b; b=c; };

        auto set_depth   = [] (address_t node, depth_t d)  { return node | address_t(d) << 8*(sizeof(address_t)-sizeof(depth_t));  };
        auto get_depth   = [] (address_t node)  { return  depth_t(node >> 8*(sizeof(address_t)-sizeof(depth_t))); };
        auto get_address = [] (address_t node)  { return  node & ((address_t(1) << 8*(sizeof(address_t)-sizeof(depth_t)))-1); };

        // WARNING !!! in order to keep low stack memory usage (important for UPMEM arch),
        // we "patch" the address value by adding the depth as the most significant byte
        // -> it makes it possible to have only one stack and not a dedicated stack for storing the node depths.
        stack_.insert (set_depth(node,depth));

        // We will use a variable that keep track of the previous node depth
        // (remember that depth=0 stands for the leaves and intermediate nodes have higher depth).
        // So when the current node depth is equal to prevDepth+1, it means that we currently traverse
        // a node whose children have just been traversed (ie. with a depth equal to prevDepth)
        uint8_t prevDepth = depth;

        // Here is our iteration. Note that we should begin the iteration with the root node present twice,
        // we iterate as long as there is only one node left in the stack (the root node).
        while (stack_.size() > 1)
        {
            // We get the top of the stack and remove it from the stack.
            address_t head = stack_.top();

            // We get the actual address and depth from it.
            node = get_address(head);

            depth_t d = get_depth(head);

            // We check whether we have finished all children.
            bool areChildrenDone = d == (prevDepth+1);

            if (node==address_t())
            {
                stack_.pop();
                continue;
            }

            // We have here a non-leaf node that has not yet traversed its children.
            if (d > 0  and  not areChildrenDone)
            {
                // We insert all the children into the stack.
                address_t* block = ALLOCATOR::read (node, stack_.tail(), NBITEMS_PER_BLOCK*sizeof(address_t));
                stack_.shift (NBITEMS_PER_BLOCK);

                // And into the depth stack as well with a lesser depth 'd-1' than the current node.
                for (size_t k=0; k<NBITEMS_PER_BLOCK; k++)   { block[k] = set_depth (block[k], d-1);  }

                // If we want insertion order, we have to revert the order of the retrieved data.
                // If we want reverse order, we don't have to do anything.
                // NOTE: NBITEMS_PER_BLOCK must be power of 2 to make 'swap' efficient.

                if (not REVERSED)
                {
                    for (size_t k=0; k<NBITEMS_PER_BLOCK/2; k++)  {  swap (block[k], block[NBITEMS_PER_BLOCK-k-1]);  }
                }
            }

            // Otherwise, it means that we have:
            //    - a leaf node
            //    - or a non-leaf node with all its children just traversed
            // We just traverse this node and remove it from the stack.
            else
            {
                // We call the functor with the retrieved value and its associated depth.
                fct (d, node);

                // We memorize the current depth for the next iteration.
                prevDepth = d;

                // This node has been processed, we remove it from the stack.
                stack_.pop();
            }
        }
    }

    // NOTE: implementation that use a distinct stack for node depths.
    template<bool REVERSED, typename FCT>  void DFS_iter2 (FCT fct, depth_t depth, address_t node)
    {
        // We first need to flush the tree -> reorganize the tree in order to have only the root node in the stack.
        flush();

        auto swap = [] (address_t& a, address_t& b)  {  address_t c(a); a=b; b=c; };

        // We need also a stack for keeping track of node depth.
        depth_t dstack [NBITEMS_PER_BLOCK * LEVEL_MAX];
        int8_t didx = 0;
        dstack[didx] = depth;

        stack_.insert (node);

        // We will use a variable that keep track of the previous node depth
        // (remember that depth=0 stands for the leaves and intermediate nodes have higher depth).
        // So when the current node depth is equal to prevDepth+1, it means that we currently traverse
        // a node whose children have just been traversed (ie. with a depth equal to prevDepth)
        uint8_t prevDepth = depth;

        // Here is our iteration. Note that we should begin the iteration with the root node present twice,
        // we iterate as long as there is only one node left in the stack (the root node).
        while (stack_.size() > 1)
        {
            // We get the top of the stack and remove it from the stack.
            node = stack_.top();

            // We also get its depth
            auto d = dstack[didx];

            // We check whether we have finished all children.
            bool areChildrenDone = d == (prevDepth+1);

            if (d > 0  and  not areChildrenDone)
            {
                // We insert all the children into the stack.
                address_t* block = ALLOCATOR::read (node, stack_.tail(), NBITEMS_PER_BLOCK*sizeof(address_t));
                stack_.shift (NBITEMS_PER_BLOCK);

                // And into the depth stack as well with a lesser depth 'd-1' than the current node.
                for (size_t k=0; k<NBITEMS_PER_BLOCK; k++)  {  dstack[++didx] = d-1;  }

                // If we want insertion order, we have to revert the order of the retrieved data.
                // If we want reverse order, we don't have to do anything.
                // NOTE: NBITEMS_PER_BLOCK must be power of 2 to make 'swap' efficient.

                if (not REVERSED)
                {
                    for (size_t k=0; k<NBITEMS_PER_BLOCK/2; k++)  {  swap (block[k], block[NBITEMS_PER_BLOCK-k-1]);  }
                }
            }

            // In such a case, it means that the current node is done, ie. all its children have
            // been processed, so we don't insert these children one more time and we just traverse this
            // node and remove it from the stack.
            else
            {
                // We call the functor with the retrieved value and its associated depth.
                fct (d, node);

                // We memorize the current depth for the next iteration.
                prevDepth = d;

                // This node has been processed, we remove it from the stack.
                stack_.pop();

                // And from the depth stack as well.
                didx--;
            }
        }
    }

    bool    isDirty_ = false;
    depth_t maxDepth_ = 0;
    uint8_t countsPerDepth_ [1+LEVEL_MAX];
    size_type  nbItems_ = 0;

    stack   stack_;

    size_type previousIdx_ = size_type(~0);
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif // __BPL_UTILS_MEMORY_TREE__

