////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#ifndef __BPL_UTILS_VECTOR__
#define __BPL_UTILS_VECTOR__

#include <cstdint>

#include <bpl/utils/MemoryTree.hpp>
#include <bpl/utils/metaprog.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

namespace impl {

template<
    typename T,
    typename ALLOCATOR,
    int NB_ITEMS  // supposed to be a power of 2
>
struct Cache
{
    static_assert ((NB_ITEMS & (NB_ITEMS - 1)) == 0);  // supposed to be a power of 2

    using address_t   = typename ALLOCATOR::address_t;
    using size_type   = uint32_t;

    static constexpr size_type MASK = (NB_ITEMS - 1);

    Cache ()              = default;
    Cache (Cache&& other) = default;
    Cache (const Cache& ) = delete;

    Cache& operator= (      Cache&& other) = default;
    Cache& operator= (const Cache&  other) = delete;

    void read   (address_t a)  { address_=a;   ALLOCATOR::read    (address_, (void*)datablock_, sizeof(datablock_));  }
    auto update ()             {        return ALLOCATOR::writeAt (address_, (void*)datablock_, sizeof(datablock_));  }
    auto write  ()             {     address_= ALLOCATOR::write   (          (void*)datablock_, sizeof(datablock_)); return address_; }

    T& operator [] (size_type  idx) const  {  return datablock_[idx & MASK];  }

    bool isFull (size_type  idx) const { return (idx & MASK) == MASK; }

    address_t address_ = 0;
    mutable T datablock_ [NB_ITEMS];

    static constexpr int SIZEOF_DATABLOCK = sizeof(datablock_);
};

}; // namespace impl

/******************************************************************************************
#     #  #######   #####   #######  #######  ######          #     #  ###  #######  #     #
#     #  #        #     #     #     #     #  #     #         #     #   #   #        #  #  #
#     #  #        #           #     #     #  #     #         #     #   #   #        #  #  #
#     #  #####    #           #     #     #  ######          #     #   #   #####    #  #  #
 #   #   #        #           #     #     #  #   #            #   #    #   #        #  #  #
  # #    #        #     #     #     #     #  #    #            # #     #   #        #  #  #
   #     #######   #####      #     #######  #     #  #####     #     ###  #######   ## ##
*******************************************************************************************/

/** 'vector_view' provides a basis for the 'vector' implementation.
 *
 * It merely provides iteration feature and can not be modified.
 * On the other hand, 'vector' implements the 'push_back' method.
 *
 * The view holds a memory address and the number of items available at this address.
 * However, the items can not be iterated directly from this address: a subset of items
 * must be retrieved first into a cache (of size DATABLOCK_SIZE). This is necessary in the
 * context of ArchUpmem for instance since the whole vector is kept in MRAM, but the viewable
 * part (ie. the cache) must be in WRAM.
 *
 * NOTE: full copy is not allowed (constructor and operator=)  but move copy is supported.
 * This is intended for having a low WRAM footprint because each instance of vector takes
 * SIZEOF_DATABLOCK bytes in WRAM. Actually, the template parameter DATABLOCK_SIZE_LOG2 that
 * determines the stack footprint of a vector_view instance has to be computed at a higher level.
 * (see for instance ArchUpmemResources). We nevertheless put here a static assert that forbids
 * the possibility to have T taking too much stack memory.
 *
 * NOTE: the cache (ie. attribute 'datablock_') is used for iterating a vector_view BUT in order
 * to have low WRAM usage, we can't not duplicate this cache. The consequence is that we can't have
 * two iterators at the same time on a vector_view instance. We will see later with class 'vector'
 * another similar restriction.
 *
 *  \tparam T the type of objects hold in the vector
 *  \tparam ALLOCATOR the allocator class used for memory management
 *  \tparam DATABLOCK_SIZE_LOG2 (log2 of) number of items in the data cache.
 */
template<
    typename T,
    typename ALLOCATOR,
    int DATABLOCK_SIZE_LOG2,
    bool SHARED_ITER_CACHE = true
>
class vector_view
{
protected:
    template<class REFERENCE> struct iterator;  // forward declaration.

public:

    static constexpr int DATABLOCK_SIZE = 1 << DATABLOCK_SIZE_LOG2;
    static constexpr int DATABLOCK_MASK = (DATABLOCK_SIZE - 1);

    static_assert (Log2<sizeof(T)>::value <= 9);  // can't cope with T type taking too much memory

    using value_type  = T;
    using size_type   = uint32_t;
    using address_t   = typename ALLOCATOR::address_t;

    /** Default constructor. We use the one generated by the compiler. */
    vector_view ()                         = default;

    /** Copy constructor. DELETED, which means that we can't fully copy a vector. */
    vector_view (const vector_view& other) = delete;

    /** Move constructor.
     * \param other: the instance we get the resources from
     */
    vector_view (vector_view&& other) = default;

    /** Copy assignment. DELETED, which means that we can't fully copy a vector through assignment. */
    vector_view& operator= (const vector_view& other) = delete;

    /** Move assignment.
     * \param other: the instance we get the resources from
     */
    vector_view& operator= (vector_view&& other) = default;

    /** Get the number of items in the vector.
     * \return the number of items. */
    size_type size() const { return datablockIdx_ + 1;  }

    /** Get an iterator on the vector that points to the beginning of the vector
     * \return the iterator. */
    auto begin() const { return iterator<vector_view> (this, 0); }

    /** Get an sentinel-like iterator on the vector for the end of the vector
     * \return the iterator. */
    auto end  () const { return size_type(size());  }

    /** The following methods are not std compliant but are required for the UPMEM architecture. */

    /** Fill the vector from some data available in memory.
     * \param address : the memory address where the data of the vector are.
     * \param nbItems : number of items of the vector
     * \param sizeItem: size of one item.
     */
    void fill (address_t address, size_t nbItems, size_t sizeItem)
    {
        datablockIdx_ = nbItems - 1;
        fillAddress_  = address;
    }

    /** Get the memory address of the vector data
     * \return the address
     */
    address_t getFillAddress() const { return fillAddress_; }

    /** Tell whether the vector has been filled, ie a call to 'fill' has already been done.
     * \return true if 'filled' has been called, false otherwise.
     */
    bool hasBeenFilled () const { return getFillAddress()!=0; }

protected:

    // The cache size is the total number of items per block divided by a power of 2
    static constexpr int CACHE_DIV_LOG2 = 1;
    static constexpr int CACHE_ITEMS_NB_LOG2  = DATABLOCK_SIZE_LOG2 - CACHE_DIV_LOG2;
    static constexpr int CACHE_ITEMS_NB       = 1 << CACHE_ITEMS_NB_LOG2;

    using cache_t = impl::Cache < T, ALLOCATOR,  CACHE_ITEMS_NB>;

    // We need a tool that provides a cache:
    //    1: a specific one or
    //    2: the one used in the vector_view itself
    // The first case is important when a vector_view is shared by several users
    // (like tasklets using a global<vector_view> instance)
    //
    // Both cases will be handled by a template specialization of this struct
    template<class REFERENCE, bool REUSE_CACHE>
    struct iterator_cache
    {
              cache_t& get(const REFERENCE* ref);
        const cache_t& get(const REFERENCE* ref) const;
    };

    template<class REFERENCE>
    struct iterator_cache<REFERENCE,true>
    {
              cache_t& get(const REFERENCE* ref)       { return ref->cache();   }
        const cache_t& get(const REFERENCE* ref) const { return ref->cache();   }
    };

    template<class REFERENCE>
    struct iterator_cache<REFERENCE,false>
    {
        iterator_cache () = default;
        iterator_cache (const iterator_cache& other) {}

              cache_t& get(const REFERENCE* ref)       { return cache_;   }
        const cache_t& get(const REFERENCE* ref) const { return cache_;   }
        cache_t cache_;
    };

    template<class REFERENCE>
    struct iterator
    {

        iterator (const REFERENCE* ref, size_type idx)  : ref_(ref), idx_(idx)
        {
            if (ref_->hasBeenFilled())
            {
                a_ = ref_->getFillAddress() + (idx_ >> DATABLOCK_SIZE_LOG2)*SIZEOF_DATABLOCK;
                cache().read (a_);
            }
        }

        bool operator != (const size_type& other) const   {  return idx_ != other;  }
        bool operator == (const size_type& other) const   {  return idx_ == other;  }

        bool operator != (const iterator& other) const   {  return idx_ != other.idx_;  }
        bool operator == (const iterator& other) const   {  return idx_ == other.idx_;  }

        const T& operator* () const  {  return cache() [idx_];  }

        iterator& operator ++ ()
        {
            if (cache().isFull(idx_++))
            {
                // We have iterated all the items available in the cache -> we must
                // fill the cache with the next data.
                a_ += SIZEOF_DATABLOCK;

                cache().read (a_);
            }
            return *this;
        }

        iterator operator+ (size_t nbItems)
        {
            iterator result { *this };

            result.a_   += (nbItems >> CACHE_ITEMS_NB_LOG2) * SIZEOF_DATABLOCK;
            result.idx_ += nbItems;

            cache().read (a_);

            return result;
        }

              cache_t& cache()       { return cache_.get(ref_); }
        const cache_t& cache() const { return cache_.get(ref_); }

        const REFERENCE* ref_;
        iterator_cache<REFERENCE,SHARED_ITER_CACHE> cache_;
        size_type  idx_ = 0;
        address_t  a_   = 0;
    };

    static_assert (Log2<sizeof(T)>::value <= 9);  // can't cope with T type taking too much memory

    // WARNING!!! order of attributes is important here
    size_type datablockIdx_  = -1;
    address_t fillAddress_   =  0;

    mutable cache_t cache_[1<<CACHE_DIV_LOG2];

    cache_t& cache(int idx=0) const { return cache_[idx]; }

    static constexpr int SIZEOF_DATABLOCK = cache_t::SIZEOF_DATABLOCK;
};

/******************************************************************************************
#     #  #######   #####   #######  #######  ######
#     #  #        #     #     #     #     #  #     #
#     #  #        #           #     #     #  #     #
#     #  #####    #           #     #     #  ######
 #   #   #        #           #     #     #  #   #
  # #    #        #     #     #     #     #  #    #
   #     #######   #####      #     #######  #     #
*******************************************************************************************/

/** The 'vector' implementation inherits from 'vector_view' and provides also 'push_back' support.
 *
 * WARNING: this implementation doesn't follow the requirement that the data is contiguous in memory.
 * The data is indeed split in chunks of DATABLOCK_SIZE elements and a tree structure is used for keeping
 * track of the order of these data chunks. This design allows to reach some kind of dynamic allocation
 * that is not so easy to support for the UPMEM architecture.
 *
 *  * NOTE: full copy is not allowed (constructor and operator=) but move copy is supported
 */
template<
    typename T,
    typename ALLOCATOR,
    int DATABLOCK_SIZE_LOG2,
    int MEMTREE_NBITEMS_PER_BLOCK_LOG2,
    int MAX_MEMORY_LOG2
>
class vector : public vector_view<T,ALLOCATOR,DATABLOCK_SIZE_LOG2>
{
private:

    using parent_t     = vector_view<T,ALLOCATOR,DATABLOCK_SIZE_LOG2>;
    using memorytree_t = MemoryTree <ALLOCATOR, MEMTREE_NBITEMS_PER_BLOCK_LOG2, MAX_MEMORY_LOG2>;

public:

    static constexpr int SIZEOF_DATABLOCK = parent_t::SIZEOF_DATABLOCK;

    static constexpr int MEMTREE_NBITEMS_PER_BLOCK  = 1 << MEMTREE_NBITEMS_PER_BLOCK_LOG2;
    static constexpr int MAX_MEMORY                 = 1 << MAX_MEMORY_LOG2;

    static constexpr int DATABLOCK_SIZE             = parent_t::DATABLOCK_SIZE;
    static constexpr int DATABLOCK_MASK             = parent_t::DATABLOCK_MASK;

    using value_type  = typename parent_t::value_type;
    using size_type   = typename parent_t::size_type;

    using address_t = typename ALLOCATOR::address_t;

    vector()  {  DEBUG_VECTOR ("[vector:: vector]  this: %p\n", this);  }

    // We don't allow to copy a vector.
    vector(const vector& other) = delete;

    vector(vector&& other) :
       parent_t(std::move(other)),
       dirty_               (other.dirty_),
       memoryTree_          (std::move(other.memoryTree_))
    {
        DEBUG_VECTOR ("[vector:: vector (move)]  this: %p  src: %p\n", this, &other);

        for (size_t i=0; i<sizeof(previousBlockIdx_)/sizeof(previousBlockIdx_[0]); i++)  { previousBlockIdx_[i] = other.previousBlockIdx_[i]; }
    }

    vector& operator= (vector&& other)
    {
        DEBUG_VECTOR ("[vector::operator=(move)]  this: %p  src: %p\n", this, &other);
        if (this != &other)
        {
            parent_t::operator=(std::move(other));
            dirty_            = other.dirty_;
            for (size_t i=0; i<sizeof(previousBlockIdx_)/sizeof(previousBlockIdx_[0]); i++)  { previousBlockIdx_[i] = other.previousBlockIdx_[i]; }
            memoryTree_       = std::move(other.memoryTree_);
        }
        return *this;
    }

    // For the moment, we avoid to copy assign vectors.
    vector& operator= (const vector& other) = delete;

    ~vector()
    {
        DEBUG_VECTOR ("[vector::~vector]  this: %p\n", this);

        if (ALLOCATOR::is_freeable == true)
        {
            memoryTree_.DFS ([&] (int8_t depth, address_t value)
            {
                // we deallocate each leaf node through a DFS traversal.
                if (depth==0)  {  ALLOCATOR::free (value);  }
            });
        }
    }

    void push_back (const T& item)
    {
        VERBOSE_VECTOR ("[vector::push_back]  idx before: %d   cachefull: %d \n", this->datablockIdx_,
            this->cache().isFull(this->datablockIdx_)
        );

        // We check that we the current data block is not full
        if (this->cache().isFull(this->datablockIdx_))
        {
            // We flush the vector (cache + memtree).
            flush_block (true);

            // We also remember the block id for a possible call to the operator []
            previousBlockIdx_[0] = (this->datablockIdx_+1) >> parent_t::CACHE_ITEMS_NB_LOG2;
        }

        // we increase the index
        this->datablockIdx_++;

        // we add the provided item in the datablock
        this->cache()[this->datablockIdx_] = item;

        this->dirty_ = true;
    }

    /** Appends a new element to the end of the container. */
    template< class... ARGS >
    void emplace_back (ARGS&&... args)
    {
        // no optimization (right now)
        this->push_back ( T(std::forward<ARGS>(args)...) );
    }

    /** Returns the (copied) value at index 'idx'
     * NOTE: this version is const BUT since we need lazy accession, it implies that some
     * attributes need to be mutable
     */

	mutable size_t idxCache_ = 0;

    value_type& operator[] (size_type idx) const
    {
        this->flush_block ();

        bool a = (idx >> parent_t::CACHE_ITEMS_NB_LOG2) != previousBlockIdx_[0];
        bool b = (idx >> parent_t::CACHE_ITEMS_NB_LOG2) != previousBlockIdx_[1];

        VERBOSE_VECTOR ("[vector::operator[]  this: %p  idx: %3d   (a,b): (%d,%d) \n", this, idx, a,b);

        [[maybe_unused]] size_type keep[] = { previousBlockIdx_[0], previousBlockIdx_[1]} ;

        // We need to get the address where the datablock needs to be retrieved from.
        address_t address = 0;

        if (    a and not b)  { idxCache_ = 1; }  // cache[1] is already available
        if (not a and     b)  { idxCache_ = 0; }  // cache[0] is already available
        if (    a and     b)  // no cache available for required block idx
        {
            // We are going to use the next cache.
            idxCache_ = (idxCache_+1) % 2;

            // we first update the current concent of this cache.
            if (previousBlockIdx_[idxCache_] != size_type(-1))
            {
                // We may have to flush the cache since we are going to import new data into it.
                if (idxCache_==0 and isDirty())  {  ((vector*)this)->flush();  }

                this->cache(idxCache_).update();
            }

            previousBlockIdx_[idxCache_] = idx >> parent_t::CACHE_ITEMS_NB_LOG2;

            if (this->hasBeenFilled())
            {
                // If the vector was filled (ie. call to 'fill'), we know that the blocks are
                // contiguous in memory, so we don't need to use the memory tree then.
                address = this->getFillAddress() + this->previousBlockIdx_[idxCache_]*SIZEOF_DATABLOCK;
            }
            else
            {
                // from the memory tree, we get the address of the block to be used
                address = memoryTree_[previousBlockIdx_[idxCache_]];
            }

            // we read the new block from the memory
            this->cache(idxCache_).read(address);
        }

        VERBOSE_VECTOR ("[vector::operator[]  this: %p  idx: %3d   (a,b): (%d,%d)  requiredBlock: %d   prevBlockIdx_: (%2d,%2d)   idxCache: %2d   DB_SIZ_L2: %d   dirty: %d  [%c] address: %ld => %3d\n",
            this, idx,
            a,b,
            (idx >> parent_t::CACHE_ITEMS_NB_LOG2),
            keep[0], keep[1],
            idxCache_,
            DATABLOCK_SIZE_LOG2,
            isDirty(),
            a and b ? 'X' : ' ',
                address,
            this->cache(idxCache_) [idx]
        );

        return this->cache(idxCache_) [idx];
    }

    /** */
    value_type& back () const  {  return (*this) [this->size()-1]; }

    /** */
    size_type max_size () const { return memoryTree_.max_size()*DATABLOCK_SIZE; }

    template<class REFERENCE>
    struct iterator : parent_t::template iterator<REFERENCE>
    {
        using vector_t = vector <T, ALLOCATOR, DATABLOCK_SIZE_LOG2, MEMTREE_NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2>;

        iterator (const REFERENCE* ref, size_type idx, typename memorytree_t::leaves_iterator it)
            : parent_t::template iterator<REFERENCE>(ref,idx), it_(it)
        {
            if (not this->ref_->hasBeenFilled())
            {
                // from the memory tree, we get the address of the block to be used
                // WARNING !!! won't work if idx!=0  (to be improved in MemoryTree)
                this->a_ = *it_;

                ref->cache().read (this->a_);
            }

            // the 'else' part should have already been handled by the parent constructor
        }

        iterator& operator ++ ()
        {
            // Here, this is quite uneasy to rely on the parent implementation, so we accept a little code duplication.
            if (this->ref_->cache().isFull(this->idx_++))
            {
                if (this->ref_->hasBeenFilled())  {  this->a_ += SIZEOF_DATABLOCK;  }
                else                              {  this->a_ = *(++it_);           }

                this->ref_->cache().read (this->a_);
            }
            return *this;
        }

        typename memorytree_t::leaves_iterator it_;
    };

    auto begin()  const
    {
        ((vector*)this)->flush();
        return iterator<vector> (this,0,this->memoryTree_.begin());
    }

    auto end  ()  const
    {
        return size_t (this->size());
    }

    void debug () const
    {
        memoryTree_.debug ();
    }

    void flush()
    {
        DEBUG_VECTOR ("[vector::flush]  this: %p\n", this);

        // We flush the current block if needed.
        flush_block();

        // We flush the memory tree as well.
        memoryTree_.flush ();
    }

    void flush_block (bool force=false) const
    {
        if (this->size()>0 and (isDirty() or force) )
        {
            VERBOSE_VECTOR ("[vector::flush_block] \n");

            // we save the data block from stack memory to main memory
            memoryTree_.insert (this->cache().write());

            dirty_ = false;
        }
    }

    void flush_block_all (bool force=true)
    {
        //if (this->size()>0 and (isDirty() or force) )
        {
            VERBOSE_VECTOR ("[vector::flush_block_all] 0\n");

            if (previousBlockIdx_[0] != size_type(-1))  {   this->cache(0).update();  }

            VERBOSE_VECTOR ("[vector::flush_block_all] 1\n");
            if (previousBlockIdx_[1] != size_type(-1))  {   this->cache(1).update();  }

          //  dirty_ = false;
        }
    }

    void fill (address_t address, size_t nbItems, size_t sizeItem)
    {
        // We first call the parent method.
        parent_t::fill (address, nbItems, sizeItem);

        // We process the specific part.
        for (int32_t nb=nbItems ; nb > 0; nb -= 1<<DATABLOCK_SIZE_LOG2)
        {
            memoryTree_.insert (address);
            address += SIZEOF_DATABLOCK;
        }

        flush();
    }

    auto& getMemoryTree() const { return memoryTree_; }

    bool isDirty() const
    {
        return dirty_;
    }

    mutable bool       dirty_            = false;
    mutable size_type  previousBlockIdx_[2] = {~size_type(0),~size_type(0)};
    mutable MemoryTree <ALLOCATOR, MEMTREE_NBITEMS_PER_BLOCK_LOG2, MAX_MEMORY_LOG2> memoryTree_;
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif // __BPL_UTILS_VECTOR__

