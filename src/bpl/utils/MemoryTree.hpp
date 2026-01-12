////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <firstinclude.hpp>

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <limits>
#include <algorithm>
#include <cmath>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** \brief Manage memory addresses as a tree.
 *
 * The idea is to insert items (like memory addresses) that can be retrieved later
 * either via operator[] or via begin/end iterators. These items are located in the
 * leaves of the tree.
 *
 * Each node of the tree is a block 2^NBITEMS_PER_BLOCK_LOG2 items.
 *
 * Each block is saved as a whole at different locations in memory.
 * However, the MemoryTree has a stack that holds information about the current
 * leaf block (ie. the one where the next 'insert' will add an item into) and its
 * parent nodes.
 *
 * The size of this class is determined by the MAX_MEMORY_LOG2 parameter,
 * i.e. the sizeof will be of order 2^MAX_MEMORY_LOG2.
 *
 * According to the choice of NBITEMS_PER_BLOCK_LOG2 and MAX_MEMORY_LOG2, the maximum
 * number of items for this structure is computed. It means that the tree can have a
 * maximum depth LEVEL_MAX.
 *
 * NOTE: we also want to implement a double linked list for the leaves of the tree.
 * The choice is made to use two of the NBITEMS_PER_BLOCK items of a leaf block,
 * one for storing the address of the previous leaf and one for storing the address
 * of the next leaf.
 *
 * \param ALLOCATOR: allocator for "dynamic allocation", for instance in MRAM
 * \param NBITEMS_PER_BLOCK_LOG2: number of items (log2) of blocks per node
 * \param MAX_MEMORY_LOG2: size in bytes (log2) that an instance of MemoryTree should take in the execution stack.
 */
template <typename ALLOCATOR, int NBITEMS_PER_BLOCK_LOG2, int MAX_MEMORY_LOG2, typename address_t = typename ALLOCATOR::address_t>
class MemoryTree
{
private:

    constexpr static uint64_t pow (uint8_t base, uint8_t exp)  {
        uint64_t res = 1;  for (uint8_t n=1; n<=exp; ++n)  { res *= base; }
        return res;
    }

public:

    // A few checks.
    static_assert (NBITEMS_PER_BLOCK_LOG2 >= 2);
    static_assert (NBITEMS_PER_BLOCK_LOG2 <= 7);

    using size_type  = uint32_t;
    using depth_t    = int8_t;

    /** Max memory (ie. stack memory) available for an instance of MemoryTree. */
    static constexpr uint32_t MAX_MEMORY = 1ULL << MAX_MEMORY_LOG2;

    static constexpr uint8_t NBITEMS_PER_BLOCK      = 1ULL << NBITEMS_PER_BLOCK_LOG2;
    static constexpr uint8_t NBITEMS_PER_BLOCK_MASK = (NBITEMS_PER_BLOCK-1);

    static constexpr size_t BLOCK_SIZEOF = NBITEMS_PER_BLOCK * sizeof (address_t);

    // Take into account the previous/next links for the double linked list
    static constexpr uint8_t LEAVES_NBITEMS_PER_BLOCK = NBITEMS_PER_BLOCK + 2;

    // Use reserved indexes inside a leaf block in order to store previous/next siblings nodes
    static constexpr size_t IDX_BLOCK_PREVIOUS = NBITEMS_PER_BLOCK+0;
    static constexpr size_t IDX_BLOCK_NEXT     = NBITEMS_PER_BLOCK+1;

    static constexpr depth_t LEVEL_MAX_THRESHOLD = 6;
    static constexpr depth_t LEVEL_MAX_FULL = 2 + MAX_MEMORY / (NBITEMS_PER_BLOCK*sizeof(address_t));

    /** Estimate of the max depth of the tree according to the max memory we can use.
     * Note that we take into account only the nodes of a path leading from the root to one leaf.
     */
    static constexpr depth_t LEVEL_MAX      = LEVEL_MAX_FULL<=LEVEL_MAX_THRESHOLD ? LEVEL_MAX_FULL : LEVEL_MAX_THRESHOLD;

    /** Maximum number of leaves. */
    static constexpr u_int64_t NBLEAVES_MAX = pow (NBITEMS_PER_BLOCK, LEVEL_MAX);

    /** 0 represents the leaves level */
    static constexpr depth_t LEAF_LEVEL = 0;

    // A few checks.
    static_assert (LEVEL_MAX >= 1);

    /** Default constructor. */
    constexpr MemoryTree()
    {
        // we reset the counts for each possible depth.
        for (size_t i=0; i<sizeof(countsPerDepth_)/sizeof(countsPerDepth_[0]); i++)  { countsPerDepth_[i] = 0; }
    }

    /** No copy constructor. */
    MemoryTree (const MemoryTree&  other) = delete;
    MemoryTree (      MemoryTree&& other) = default;

    /** No assignment operator. */
    MemoryTree& operator= (const MemoryTree&  other) = delete;
    MemoryTree& operator= (      MemoryTree&& other) = default;

    /** Destructor. Note that the memory might not be release according to the feature of the allocator. */
    constexpr ~MemoryTree()
    {
        //DEBUG_MEMTREE ("[MemoryTree::~MemoryTree] this: 0x%lx  #stack=%ld  depth=%d \n", this, stack_.size(), depth());

        if constexpr (ALLOCATOR::is_freeable)
        {
            iterate_stack ([&](size_t depth, address_t value)
            {
                // We skip leaves here.
                if (depth==LEAF_LEVEL)  { return false; }

                // We iterate each non leaf node in the stack
                DFS_iter<false> ([&] (depth_t depth, address_t value)
                {
                    if (depth != LEAF_LEVEL)  {  ALLOCATOR::free (value);  }
                }, depth, value);

                return true;
            });
        }
    }

    /** Insert a new address as a leaf of the tree. When the current block is full, the tree is re-organized.
     * After an insertion, the tree can be in a "dirty" state and a flush is potentially processed by other methods.
     * \param a: address to be added to the tree.
     * \return the inserted address.
     */
    auto insert (address_t a)
    {
        DEBUG_MEMTREE("[MemoryTree::insert] this: 0x%lx   a=0x%lx    bounds: [0x%lx,0x%lx]  \n",
            uint64_t(this),
            uint64_t(a),
            uint64_t(leavesBounds_.first),
            uint64_t(leavesBounds_.second)
        );

        // we increase the number of inserted leaves.
        nbLeaves_ ++;

        previousBlockIdx_ = nbLeaves_ >> NBITEMS_PER_BLOCK_LOG2;

        // we insert the item into the stack.
        stack_.insert (a);

        // we increase the number of items for depth 0 (ie. leaves)
        countsPerDepth_[LEAF_LEVEL] ++;

        // We may have to flush the content of the stack. We begin with leaf level and go upwards in the tree.
        // Each full block will be merged, ie. copied in memory at a given address that will be registered
        // in the parent block.
        flush ([&](depth_t depth)  {  return countsPerDepth_[depth] == NBITEMS_PER_BLOCK;  });

        // we set the tree as "dirty" -> other methods could need to do a 'flush'
        isDirty_ = true;

        return a;
    }

    /** Insert 'nbitems' nodes every 'size' bytes, starting at address 'start'
     * \param start: starting address
     * \param nbitems: number of items to be inserted
     * \param size: size of each item to be inserted.
     */
    auto insert (address_t start, size_t nbitems, size_t size)
    {
        for (size_t nb=0; nb<nbitems; nb++)
        {
            this->insert (start);
            start += size;
        }
    }

    /** Access to a specific item given its index.
     * We do a traversal of the tree from the root to the target leaf, so we need to ask in memory the children of a node
     * but we can avoid some memory accesses in a few cases.
     * \param idx: index of the item to be retrieved
     * \return the retrieved item
     */
    address_t operator[] (size_t idx)
    {
        address_t result {};

        DEBUG_MEMTREE ("\n----------------------------------------------------------------------------------------\n");

        DEBUG_MEMTREE ("Memtree operator[] idx: %ld  BlockIdx: [%ld,%ld]  StackLeavesNb: %ld   case: %ld\n",
            (uint64_t)idx,
            (uint64_t)idx>>NBITEMS_PER_BLOCK_LOG2,
            (uint64_t)previousBlockIdx_,
            (uint64_t)getStackLeavesNb(),
            (uint64_t) (idx>>NBITEMS_PER_BLOCK_LOG2 == size()>>NBITEMS_PER_BLOCK_LOG2)
        );
        debug("DEBUG FOR operator[]");

        address_t* tail = stack_.tail();
        int notaligned = uint64_t(tail) % 8 != 0;

        if (notaligned)  { tail = (address_t*) (uint64_t(tail+7) & ~7); }

        // The required block may be already there (in the cache)
        if (idx>>NBITEMS_PER_BLOCK_LOG2 == size()>>NBITEMS_PER_BLOCK_LOG2 )
        {
            result = getStackLeaves() [idx & NBITEMS_PER_BLOCK_MASK];
        }

        // The required block may be already there (after the cache)
        else if (idx>>NBITEMS_PER_BLOCK_LOG2 == previousBlockIdx_)
        {
            result = tail [idx & NBITEMS_PER_BLOCK_MASK];
        }
        else
        {
            /// We may need to normalize the tree.
            normalize();

            size_t    requiredBlockIdx = 0;
            size_t    requiredDepth    = 0;
            iterate_stack ([&](size_t depth, address_t value)
            {
                size_t p=1;
                for (size_t d=1; d<=depth; d++)  { p *= NBITEMS_PER_BLOCK; }
                requiredBlockIdx += p;
                requiredDepth = depth;
                result        = value;
                return idx >= requiredBlockIdx;
            });

            // accessing the required leaf consists in traversing the tree like we enumerate the digits of a number in a given basis.
            for (depth_t power=requiredDepth-1; power>=0; power--)
            {
                size_t digit = (idx >> (power*NBITEMS_PER_BLOCK_LOG2)) & NBITEMS_PER_BLOCK_MASK;

                // we read a block from memory and put it at the end of the stack.
                // note: we use the inner buffer of the stack here, ie. we don't use the 'insert' method
                // -> we can do that because we have had a supplementary level during the stack declaration
                // which allows to address NBITEMS_PER_BLOCK items
                // Note also that we want to go directly to a specific leaf, so we don't need to keep information
                // of intermediate nodes.
                address_t* ptr = ALLOCATOR::read ((void*)result , tail, sizeof(result)*(NBITEMS_PER_BLOCK));

                DEBUG_MEMTREE ("Memtree operator[]  idx=%ld  power=%ld  digit=%ld  #stack=%ld  result: 0x%lx  ptr: 0x%lx  tail: 0x%lx  notaligned: %ld\n",
                    (uint64_t)idx,
                    (uint64_t)power,
                    (uint64_t)digit,
                    (uint64_t)stack_.idx_,
                    (uint64_t)result,
                    (uint64_t)ptr,
                    (uint64_t)tail,
                    (uint64_t)notaligned
                );

                // we update the result.
                result = ptr[digit /* + (notaligned!=0 ? 1 : 0)*/];
            }

            // we remember the required idx for potentially optimize lookup for next call to operator[]
            previousBlockIdx_ = idx >> NBITEMS_PER_BLOCK_LOG2;

            DEBUG_MEMTREE ("Memtree operator[]  previousBlockIdx_: %ld  result: 0x%lx \n",
                (uint64_t)previousBlockIdx_,
                (uint64_t)result
            );
        }

        return result;
    }

    /** "ALMOST" Depth First Search of the tree.
     *  WARNING!!! this is not a "true" DFS, but the important fact is that leaves will
     *  be iterated in the same order than the one used for their insertion into the tree.
     *  The provided functor is called for each node.
     *  \param fct: functor called for each item. */
    template<typename FCT>  void DFS (FCT fct)
    {
        // Nothing to do for an empty tree.
        if (size()==0)  { return; }

        // The stack should have only the root now.
        DFS_iter<false> (fct, depth(), stack_.top());
    }

    ////////////////////////////////////////////////////////////////////////////////
    /**  Iterate each value of the stack with its associated depth
     * \param fct: functor called with (1) the depth in the stack and (2) the
     * current item
     */
    template<typename FCT>
    auto iterate_stack (FCT fct)
    {
        bool result = true;

        if (stack_.idx_==0)  { return result; }  // needed if tree is empty

        depth_t depth = maxDepth_;

        size_t i=0;
        for (size_t count = countsPerDepth_ [depth]; depth >= 0; ++i)
        {
            // we notify the depth and the item of the stack.
            result = fct (depth, stack_.items_[i]);
            if (result==false)  { break; }

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
        return result;
    }

    /** Number of leaves in the tree.
     * \return Nodes number. */
    size_type size()  const { return nbLeaves_; }

    /** \return maximum number of leaves possible for this memory tree. */
    uint64_t max_size () const { return NBLEAVES_MAX; }

    /** Depth of the tree.
     * \return the depth. */
    depth_t depth() const { return maxDepth_ ; }

    /** Debug utility.
     */
    void debug(const char* txt="tree") const
    {
        DEBUG_MEMTREE ("%s: ", txt);
        DEBUG_MEMTREE (" LEV_MAX=%ld  NBITBLOCKS=%ld  sizeof=%ld  #items=%4ld  depth=%2ld  #stack=%2ld  isDirty_=%ld  top=0x%lx  maxsize: %ld  counts:  ",
            (uint64_t)LEVEL_MAX,
            (uint64_t)NBITEMS_PER_BLOCK,
            (uint64_t)sizeof(*this),
            (uint64_t)size(),
            (uint64_t)depth(),
            (uint64_t)stack_.size(),
            (uint64_t)isDirty_,
            (uint64_t)(stack_.size()>0 ? stack_.top() : -1),
            (uint64_t)max_size()
        );
        for (depth_t i=0; i<=maxDepth_; i++)
        {
            DEBUG_MEMTREE ("(%ld:%2ld)  ", uint64_t(i), uint64_t(countsPerDepth_[i]));
        }

        for (size_t i=0; i<stack_.size(); i++)  {  DEBUG_MEMTREE ("[0x%lx] ", uint64_t(stack_[i])); }

        DEBUG_MEMTREE ("\n");
    }

    /** Debug utility. */
    auto dumpstack (const char* msg="dumpstack")
    {
        DEBUG_MEMTREE ("[%s]  maxDepth_: %ld  #stack: %ld\n", msg, uint64_t(maxDepth_),  uint64_t(stack_.size()));
        int k=0;
        for (int i=maxDepth_; i>=0; i--)
        {
            for (int l=0; l<countsPerDepth_[i]; l++, k++)
            {
                DEBUG_MEMTREE ("   stack[%2ld]:  level: %ld   address: 0x%lx\n",
                    (uint64_t)k,
                    (uint64_t)i,
                    (uint64_t)stack_[k]
                );
            }
        }
    }

    /** Update the counts for the different depths, starting by the leaves (ie. depth==0)
     * \param check: the functor telling whether the current depth has to be merged or not.
     */
    template<typename CHECK>
    void flush (CHECK check)
    {
        for (depth_t depth=LEAF_LEVEL; depth<=maxDepth_; depth++)
        {
            if (check(depth)) {  merge (depth);  }
        }
    }

    /** Normalization of the tree.
     */
    void normalize ()
    {
        if (isDirty_)
        {
            flush ([&](depth_t depth)  {  return countsPerDepth_[depth] == NBITEMS_PER_BLOCK;  });
            isDirty_ = false;
        }
    }

    /** Facade method for optimization. See the overload with the IsLeaf boolean */
    auto merge (depth_t depth)
    {
        if (depth==LEAF_LEVEL)  { merge(depth, true ); }
        else                    { merge(depth, false); }
    }

    /** Allocate some memory and copy a block into it.
     * The tree structure is also updated. */
    auto merge (depth_t depth, bool IsLeaf)
    {
        constexpr int32_t  NbItemsToWrite = std::max(LEAVES_NBITEMS_PER_BLOCK, NBITEMS_PER_BLOCK);

        size_t nbItems = countsPerDepth_[depth];

        // we add empty values to the end of the stack in order to have a complete block
        for (size_t i=nbItems ; i < NbItemsToWrite; i++)  {  stack_.insert (address_t{}); }

        // At leaf level, we update the previous link
        if (IsLeaf)
        {
            if (leavesBounds_.second != 0)  {  stack_.get(-NbItemsToWrite)[IDX_BLOCK_PREVIOUS] = leavesBounds_.second;  }
        }

        address_t* from = stack_.get(-NbItemsToWrite);

        address_t b = 0;


        if (uint64_t(from)%8 == 0)
        {
            b = ALLOCATOR::write (from, sizeof(address_t) * NbItemsToWrite);
        }
        else
        {
            // Enforce alignment
            address_t tmp [NbItemsToWrite];
            for (int i=0; i<NbItemsToWrite; i++)  {  tmp[i] = from[i];  }

            b = ALLOCATOR::write (tmp, sizeof(address_t) * NbItemsToWrite);
        }

        DEBUG_MEMTREE ("[MemoryTree::merge]  from: 0x%lx [%c]  bounds: [0x%lx,0x%lx]  #items: %ld  depth: %ld  block of %2ld written at 0x%lx -> ",
            uint64_t(from),
            uint64_t(from)%8==0 ? ' ' : '!',
            uint64_t(leavesBounds_.first),
            uint64_t(leavesBounds_.second),
            uint64_t(nbItems),
            uint64_t(depth),
            uint64_t(NbItemsToWrite),
            uint64_t(b)
        );
        for (size_t i=0; i<NbItemsToWrite; i++)  {  DEBUG_MEMTREE ("0x%lx  ", uint64_t(stack_.get(-NbItemsToWrite)[i]));  }
        DEBUG_MEMTREE ("\n");

        // At leaf level, we update the next link
        if (IsLeaf)
        {
            if (leavesBounds_.second != 0)
            {
                // we update the next link of the previous leaf
                address_t* ptr = (address_t*)leavesBounds_.second + IDX_BLOCK_NEXT;

                ALLOCATOR::writeAtomic ((address_t)ptr, b);

                DEBUG_MEMTREE ("[MemoryTree::merge] MEMORY UPDATE FOR NEXT LINK: 0x%lx (0x%lx)  -> 0x%lx   IDX_BLOCK_NEXT: %ld \n",
                    uint64_t(leavesBounds_.second),
                    uint64_t(ptr),
                    uint64_t(b),
                    uint64_t(IDX_BLOCK_NEXT)
                );
            }

            // We remember the last seen leaf.
            leavesBounds_.second = b;
            DEBUG_MEMTREE ("[MemoryTree::merge] SETTING LAST : 0x%lx\n", uint64_t(b));

            // We remember the first leaf.
            if (leavesBounds_.first==0)
            {
                leavesBounds_.first = b;
                DEBUG_MEMTREE ("[MemoryTree::merge] SETTING FIRST: 0x%lx\n", uint64_t(b));
            }
        }

        // we pop this block of addresses from the stack
        stack_.pop (NbItemsToWrite);

        // we insert the new block.
        stack_.insert (b);

        // we update the counts for the current level (goes back to 0) and increase the next one.
        countsPerDepth_[depth+0]  = 0;
        countsPerDepth_[depth+1] += 1;

        if (maxDepth_ < depth+1)  { maxDepth_ = depth+1; }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /** Iteration of the leaves of the tree.
     * \param FORWARD: true:=forward iteration, backward otherwise
     * \param fct: functor called for each node to be iterated.
     * \*/
    template<bool FORWARD, typename FCT>
    auto iterate_leaves (FCT fct)
    {
        auto iterate = [&] (auto begin, auto end)  {  for (auto it=begin; it!=end; ++it)  { fct(*it); }  };

        if constexpr (FORWARD)  { iterate ( begin(),  end()); }
        else                    { iterate (rbegin(), rend()); }
    }

    /** Iteration over the leaves of the tree.
     * \param fct: functor called over each leaf of the tree.
     */
    template<typename FCT>  auto leaves (FCT fct)
    {
        return iterate_leaves<true> (fct);
    }

    ////////////////////////////////////////////////////////////////////////////////
    /** Iterator class over the leaves of a tree (forward or backward)
     */
    template<bool FORWARD>
    struct leaves_iterator_generic
    {
        using tree_t = MemoryTree<ALLOCATOR,NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2>;

        // We may have to use the content of the stack
        address_t* stackLeaves = nullptr;

        // Number of leaves present in the stack.
        size_t nbStackLeaves = 0;

        address_t*  leavesStorage = nullptr;

        // Pointer holding the information to be iterated; it may point to the stack or to the storage
        // specific to the iterator.
        address_t* storage     = nullptr;

        // We need to know what is the next block of information to be used.
        address_t  next  = 0;

        // i/end are the indexes bounds to be used for iterating 'storage'
        ssize_t i        = 0;
        ssize_t end      = 0;

        // Number of call to operator++.  Will be used for operator!=
         size_t n        = 0;
         size_t nbleaves = 0;

        const size_t link   = FORWARD ? IDX_BLOCK_NEXT : IDX_BLOCK_PREVIOUS;
        const int direction = FORWARD ? 1 : -1;

        address_t* round (address_t* a)  {  return (address_t*) ((uint64_t(a) + 7) & ~7);  }

        leaves_iterator_generic (tree_t& ref) : nbleaves(ref.size())
        {
            stackLeaves   = ref.stack_.tail() - ref.countsPerDepth_[LEAF_LEVEL];
            leavesStorage = round (ref.stack_.tail());

            // We remember the number of leaves stored in the stack.
            nbStackLeaves = ref.countsPerDepth_[LEAF_LEVEL];

            // According to FORWARD, we get the head or the tail  of the double linked list encoded with the leaves of the tree
            address_t bound = FORWARD ? ref.leavesBounds_.first : ref.leavesBounds_.second;

            DEBUG_MEMTREE ("[MemoryTree::iterator::iterator] @ref: 0x%lx  bounds: [0x%lx,0x%lx]   it: 0x%lx  i: %ld  nbleaves: %ld  LEAVES_NBITEMS_PER_BLOCK: %ld  nbStackLeaves: %ld   stackLeaves: 0x%lx  leavesStorage: 0x%lx  #stack: %ld\n",
                uint64_t(&ref),
                (uint64_t)ref.leavesBounds_.first,
                (uint64_t)ref.leavesBounds_.second,
                (uint64_t)bound,
                (uint64_t)i,
                (uint64_t)nbleaves,
                (uint64_t)LEAVES_NBITEMS_PER_BLOCK,
                (uint64_t)nbStackLeaves,
                (uint64_t)stackLeaves,
                (uint64_t)leavesStorage,
                (uint64_t)ref.stack_.size()
            );

            if (nbleaves>0)  {  configure <true> (bound);  }
        }

        leaves_iterator_generic (leaves_iterator_generic const& other) = default;
        leaves_iterator_generic (leaves_iterator_generic     && other) = default;

        leaves_iterator_generic& operator= (leaves_iterator_generic const& other) = default;
        leaves_iterator_generic& operator= (leaves_iterator_generic     && other) = default;

        bool operator!= (size_t sentinel) const {  return n != sentinel;  }

        address_t operator* () const  {  return storage[i];  }

        auto& operator++ ()
        {
            i+=direction; n++;

            if (i==end)   {  configure<false> (next);  }

            return *this;
        }

        template<bool FIRST>
        void configure_forward (address_t block)
        {
            if (block!=0)
            {
                storage = leavesStorage;

                ALLOCATOR::read ((void*)block, storage, LEAVES_NBITEMS_PER_BLOCK*sizeof(address_t));

                i      = 0;
                end    = NBITEMS_PER_BLOCK;
                next   = storage[link]!=0 ? storage[link] : 0;

                DEBUG_MEMTREE ("[MemoryTree::configure_forward]  block: 0x%lx  storage: 0x%lx  size: %ld  [i,end]: %ld %ld \n",
                    uint64_t((void*)block),
                    uint64_t(storage),
                    uint64_t(LEAVES_NBITEMS_PER_BLOCK*sizeof(address_t)),
                    uint64_t(i),
                    uint64_t(end)
                );
            }
            else
            {
                storage = stackLeaves;

                DEBUG_MEMTREE ("[MemoryTree::configure_forward]  DUMP STACK: \n");

                i      = 0;
                end    = nbleaves % NBITEMS_PER_BLOCK;
                next   = 0;
            }
        }

        template<bool FIRST>
        void configure_backward (address_t block)
        {
            if constexpr (FIRST)
            {
                // The storage is the current leaves stack.
                storage  = stackLeaves;

                if (nbStackLeaves==0)  {  ALLOCATOR::read ((void*)block, storage, LEAVES_NBITEMS_PER_BLOCK*sizeof(address_t));  }

                // We compute the index bounds
                i   = (nbleaves-1) % NBITEMS_PER_BLOCK;
                end = -1;

                // Next storage.
                // Note: if nbStackLeaves==0, it means that the previous block has been flushed
                // BUT the stack still holds the leaves => we can then avoid to read again this block.
                next = nbStackLeaves==0 ? storage[link] : block;
            }
            else if (next != 0)
            {
                // The storage is beyond the current leaves stack.
                storage = leavesStorage;

                ALLOCATOR::read ((void*)block, storage, LEAVES_NBITEMS_PER_BLOCK*sizeof(address_t));

                // We compute the index bounds
                i   = NBITEMS_PER_BLOCK-1;
                end = -1;

                // Next storage block (previous or next block in the double linked list).
                next = storage[link];
            }
        }

        template<bool FIRST>
        void configure (address_t block)
        {
            DEBUG_MEMTREE ("[MemoryTree::configure 1] FIRST: %ld  block: 0x%08lx  next: 0x%08lx  nbStackLeaves: %ld  stackLeaves: 0x%lx  leavesStorage: 0x%lx\n",
                (uint64_t)FIRST,
                (uint64_t)block,
                (uint64_t)next,
                (uint64_t)nbStackLeaves,
                (uint64_t)stackLeaves,
                (uint64_t)leavesStorage
            );

            if constexpr(FORWARD)  { configure_forward <FIRST>(block); }
            else                   { configure_backward<FIRST>(block); }

            DEBUG_MEMTREE ("[MemoryTree::configure 2] FIRST: %ld  block: 0x%08lx  next: 0x%08lx  nbStackLeaves: %ld  storage: 0x%lx ",
                (uint64_t)FIRST,
                (uint64_t)block,
                (uint64_t)next,
                (uint64_t)nbStackLeaves,
                (uint64_t)storage
            );

            DEBUG_MEMTREE (" -> 0x%lx  0x%lx  0x%lx  0x%lx  0x%lx  0x%lx \n",
                (uint64_t)storage[0],
                (uint64_t)storage[1],
                (uint64_t)storage[2],
                (uint64_t)storage[3],
                (uint64_t)storage[4],
                (uint64_t)storage[5]
            );
        }
    };

    using leaves_iterator = leaves_iterator_generic<true>;

    auto begin()   { return leaves_iterator_generic<true>(*this); }
    auto end  ()   { return size(); }

    auto rbegin()  { return leaves_iterator_generic<false>(*this); }
    auto rend  ()  { return size(); }

    ////////////////////////////////////////////////////////////////////////////////
    /** Stack structure which memorizes addresses at different tree depth.
     */
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

        address_t& operator[] (size_t i)  { return items_[i]; }

        address_t operator[] (size_t i) const { return items_[i]; }

        /** same convention as python for index i, i.e. -1 corresponds to the last item. */
        /** Don't use ssize_t for the input parameter => LEAD TO WRONG RESULTS... */
        address_t* get (int32_t i)  { return & items_[i>=0 ? i : idx_ + i ]; }

        // top
        address_t top()  const { return  items_[idx_-1]; }

        // end of the stack.
        address_t* tail ()  { return & items_[idx_]; }

        uint32_t  idx_ = 0;

       ARCH_ALIGN typename ALLOCATOR::template address_array_t<
              NBITEMS_PER_BLOCK * (LEVEL_MAX-1)  // for intermediate nodes
            + LEAVES_NBITEMS_PER_BLOCK           // for leaves
            + NBITEMS_PER_BLOCK + 1              // extra room after tail
        > items_ = {};

    }; // end of struct stack

    size_t getStackLeavesNb() const { return countsPerDepth_[LEAF_LEVEL]; }

    address_t* getStackLeaves()
    {
        return getStackLeavesNb()==0 ? nullptr : stack_.get(-countsPerDepth_[LEAF_LEVEL]);
    }

    /** Depth first search of the tree. Relies on the stack structure.
     */
    template<bool REVERSED, typename FCT>  void DFS_iter (FCT fct, depth_t depth, address_t node)
    {
        auto swap = [] (address_t& a, address_t& b)  {  address_t c(a); a=b; b=c; };

        auto set_depth   = [] (address_t node, depth_t d)  { return node | address_t(d) << 8*(sizeof(address_t)-sizeof(depth_t));  };
        auto get_depth   = [] (address_t node)  { return  depth_t(node >> 8*(sizeof(address_t)-sizeof(depth_t))); };
        auto get_address = [] (address_t node)  { return  node & ((address_t(1) << 8*(sizeof(address_t)-sizeof(depth_t)))-1); };

        size_t keepSize = stack_.size();

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
        while (stack_.size() > keepSize)
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
                address_t* block = ALLOCATOR::read ((void*)node, stack_.tail(), NBITEMS_PER_BLOCK*sizeof(address_t));
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

    bool       isDirty_ = false;
    depth_t    maxDepth_ = 0;
    uint8_t    countsPerDepth_ [LEVEL_MAX+1];
    size_type  nbLeaves_ = 0;
    stack      stack_;
    size_type  previousBlockIdx_ = -1;

    std::pair<address_t,address_t> leavesBounds_ = {0,0};
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
