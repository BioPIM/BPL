////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

#include <bpl/utils/MemoryTree.hpp>
#include <bpl/utils/metaprog.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

namespace impl {

template<
    typename T,
    typename ALLOCATOR,
    int NB_ITEMS  // supposed to be a power of 2
>
struct Cache
{
    //static_assert ((NB_ITEMS & (NB_ITEMS - 1)) == 0);  // supposed to be a power of 2

    using address_t   = typename ALLOCATOR::address_t;
    using  size_type   = uint32_t;
    using ssize_type   =  int32_t;

    static constexpr size_type MASK = (NB_ITEMS - 1);

    static constexpr size_type NbItems = NB_ITEMS;

    Cache ()              = default;
    Cache (Cache&& other) = default;
    Cache (const Cache& ) = delete;

    Cache& operator= (      Cache&& other) = default;
    Cache& operator= (const Cache&  other) = delete;

    auto read   (address_t a, address_t begin=0, address_t end=0)  {
        address_= a; boundaries_[0]=begin; boundaries_[1]=end;
        ALLOCATOR::read    ((void*)address_, (void*)datablock_, sizeof(datablock_));
        dump("READ  ");
    }

    template<int K>
    auto readNext   ()  {   read (address()+ K*SIZEOF_DATABLOCK);  }

    template<int K, auto cond=[] (ssize_type){ return true; }>
    auto updateIfFull (ssize_type  idx) {
        if (isFull(idx) and cond(idx))  {  readNext<K>(); }
    }

    auto update  ()            {
        dump("UPDATE");
        if (boundaries_[1]==0) {
            return ALLOCATOR::writeAt ((void*)address_, (void*)datablock_, sizeof(datablock_));
        }
        else {
            auto a0 = std::max (address_, boundaries_[0]);
            auto a1 = std::min (address_+sizeof(datablock_), boundaries_[1]);
            return ALLOCATOR::writeAt (
                (void*) a0,
                (void*)datablock_,
                (a1-a0)
            );
        }
    }

    auto write   ()            { dump("WRITE");        address_= ALLOCATOR::write   (                 (void*)datablock_, sizeof(datablock_)); return address_; }

    auto acquire (size_type nbitems=0) {
        dump("ACQUIRE");
        // By default, we get a size equal to the cache size.
        size_type size = (1+nbitems/NB_ITEMS)*SIZEOF_DATABLOCK;
        address_= ALLOCATOR::get(size);
        for (int i=0; i<NB_ITEMS; i++) { datablock_[i] = {}; }
        return address_;
    }

    T& operator [] (size_type  idx) const  {  return datablock_[idx & MASK];  }

    bool isFull (size_type  idx) const { return (idx & MASK) == MASK; }

    auto address() const { return address_; }

    auto dump (const char* msg)
    {
        DEBUG_VECTOR ("[%s this: 0x%lx  address: 0x%04lx:  ", msg, uint64_t(this), uint64_t(address_));
        for (int i=0; i<NB_ITEMS; i++)  {   DEBUG_VECTOR ("%3ld ", uint64_t(datablock_[i])); }
        DEBUG_VECTOR ("]\n");
    }

    static size_type getBlockIdx (size_type idx)  {
        // We might optimize the computation in case the number of items is a power of two.
        if constexpr ((NB_ITEMS & (NB_ITEMS - 1)) == 0)  {
            return idx >> bpl::Log2<NB_ITEMS>::value;
        }
        else {
            return idx / NB_ITEMS;
        }
    }

    void reset_state()  { address_ = 0; }

    ARCH_ALIGN mutable T datablock_[NB_ITEMS];

    ARCH_ALIGN address_t address_ = 0;
    ARCH_ALIGN address_t boundaries_[2] = {0,0};

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
 * must be retrieved first into a cache (of size MEMORY_SIZE). This is necessary in the
 * context of ArchUpmem for instance since the whole vector is kept in MRAM, but the viewable
 * part (ie. the cache) must be in WRAM.
 *
 * NOTE: full copy is not allowed (constructor and operator=)  but move copy is supported.
 * This is intended for having a low WRAM footprint because each instance of vector takes
 * SIZEOF_DATABLOCK bytes in WRAM. Actually, the template parameter MEMORY_SIZE_LOG2 that
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
 *  \tparam MEMORY_SIZE_LOG2 (log2 of) number of items in the data cache.
 */
template<
    typename T,
    typename ALLOCATOR,
    typename MUTEX,
    int  MEMORY_SIZE_LOG2,          // log2 of the (stack) memory size available for an instance of this class
    int  CACHE_NB_LOG2,             // log2 of the number of caches
    bool SHARED_ITER_CACHE          // true means that only one iterator can be used at the same time
>
class vector_view
{
protected:
public:
template<class REFERENCE, bool FORWARD> struct iterator;  // forward declaration.

public:

    static constexpr bool is_shared_iter_cache = SHARED_ITER_CACHE;

    static constexpr bool parseable = false;

    using type = T;

    static constexpr int MEMORY_SIZE      = 1 << MEMORY_SIZE_LOG2;  // memory size (in bytes) available for one instance
    static constexpr int MEMORY_SIZE_MASK = (MEMORY_SIZE - 1);

//    static_assert (Log2(sizeof(T)) <= 9);  // can't cope with T type taking too much memory

    // the number of caches
    static constexpr int CACHE_NB = 1 << CACHE_NB_LOG2;
    static_assert ( (CACHE_NB_LOG2 >= 0) && (CACHE_NB_LOG2 <= 1));

    // (log2) the memory available for one cache.
    // It should hold at least one item so we might have to correct the actual size
    static constexpr int CACHE_MEMORY_SIZE_LOG2  =
        (1UL << (MEMORY_SIZE_LOG2 - CACHE_NB_LOG2)) >= sizeof(T) ?
        MEMORY_SIZE_LOG2 - CACHE_NB_LOG2 :
        bpl::Log2<std::bit_ceil(sizeof(T))>::value;

    static constexpr int CACHE_MEMORY_SIZE  = 1UL << CACHE_MEMORY_SIZE_LOG2;

    // the number of items per cache
    static constexpr int CACHE_NB_ITEMS = CACHE_MEMORY_SIZE / sizeof(T);
    static_assert (CACHE_NB_ITEMS>0);

    using value_type  = T;
    using size_type   = int32_t;
    using address_t   = typename ALLOCATOR::address_t;

    using const_iterator         = iterator<vector_view,true>;
    using const_reverse_iterator = iterator<vector_view,false>;

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

    /** Forbid to get the address. */
    auto* operator& () = delete;

    /** Get the number of items in the vector.
     * \return the number of items. */
    size_t size() const { return datablockIdx_ + 1;  }

    /** Get an iterator on the vector that points to the beginning of the vector
     * \return the iterator. */
    auto begin() const { return iterator<vector_view,true> (this, 0, true); }

    /** Get an sentinel-like iterator on the vector for the end of the vector
     * \return the iterator. */
    //auto end  () const { return size_type(size());  }
    auto end() const { return iterator<vector_view,true> (this, size_type(size()), false); }

    /** */
    auto rbegin() const { return iterator<vector_view,false> (this, size_type(size())-1, true); }

    /** */
    auto rend  () const { return size_type(-1);  }

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

    /** We need a (non standard) method for resetting the internals of the object.
     * Normally in DPU code, we could use "std::move" and assign operator in order to
     * reset the guts of the object BUT it leads to create a temporary object that increases
     * stack memory usage, which is a potential issue on DPU and low WRAM capacity.
     */
    void reset_state()
    {
        for (auto& c : cache_)  { c.reset_state(); }
        datablockIdx_  = -1;
        fillAddress_   =  0;
        currentBlock_  = -1;
    }

    value_type const& operator[] (size_type idx) const
    {
        // We may need synchronization. Note that 'empty' impl won't do any synchronization.
        mutex_.lock();

        // we get the block index where the required item should be.
        auto block = cache_t::getBlockIdx(idx);

        if ((size_type)block != currentBlock_)
        {
            // we remember the current block.
            currentBlock_ = block;

            // we read the new block from the memory
            this->cache().read (this->getFillAddress() + currentBlock_*SIZEOF_DATABLOCK);
        }

        mutex_.unlock();

        return this->cache() [idx];
    }

    using cache_t = impl::Cache < T, ALLOCATOR,  CACHE_NB_ITEMS>;

    // We need a tool that provides a cache:
    //    1: a specific one or
    //    2: the one used in the vector_view itself
    // The first case is important when a vector_view is shared by several users
    // (like tasklets using a global<vector_view> instance)
    //
    // Both cases will be handled by a template specialization of this struct
    template<class REFERENCE, bool REUSE_CACHE>
    struct iterator_cache {};

    template<class REFERENCE>
    struct iterator_cache<REFERENCE,true> : std::true_type
    {
              auto& get(const REFERENCE* ref)       { return ref->cache();   }
        const auto& get(const REFERENCE* ref) const { return ref->cache();   }

        using cache_type = cache_t;
    };

    template<class REFERENCE>
    struct iterator_cache<REFERENCE,false> : std::false_type
    {
        iterator_cache () = default;
        iterator_cache (const iterator_cache& other) {}

              auto& get(const REFERENCE* ref)       { return cache_;   }
        const auto& get(const REFERENCE* ref) const { return cache_;   }

        // For not shared iterators, we use a smaller cache than the one used for the vector itself.
        //using cache_type = impl::Cache < T, ALLOCATOR,  std::max<int> (1, cache_t::NbItems/4)>;
        using cache_type = cache_t;
        cache_type cache_;
    };

    template<class REFERENCE, bool FORWARD>
    struct iterator
    {
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = long;
        using pointer           = value_type*;
        using reference         = value_type const&;

        constexpr static int K = FORWARD ? 1 : -1;

        iterator() = default;

        iterator (const REFERENCE* ref, size_type idx, bool isBegin)  : ref_(ref), idx_(idx), isBegin_(isBegin)
        {
            if (ref_->hasBeenFilled() and isBegin_ and idx_>=0)  {
                cache().read (ref_->getFillAddress() + cache_type::getBlockIdx(idx_)*cache_type::SIZEOF_DATABLOCK);
            }
        }

        iterator (iterator const& other) : iterator(other.ref_, other.idx_, other.isBegin_) { }
        iterator (iterator     && other) : iterator(other.ref_, other.idx_, other.isBegin_) { }

        iterator& operator= (iterator const& other) = delete;

        bool operator != (const size_type& other) const   {  return idx_ != other;  }
        bool operator == (const size_type& other) const   {  return idx_ == other;  }

        bool operator != (const iterator& other) const   {  return idx_ != other.idx_;  }
        bool operator == (const iterator& other) const   {  return idx_ == other.idx_;  }

        reference operator* () const  {  return cache() [idx_];  }

        template<bool forward>
        iterator& next ()  {
            // For optimization, we add an extra test only in one constexpr case.
            auto cond = [] (auto idx) { return idx>=0; };
            if constexpr (forward)  { cache().template updateIfFull<K     >(  idx_++); }
            else                    { cache().template updateIfFull<K,cond>(--idx_  ); }
            return *this;
        }

        iterator& operator ++ ()  {  return next<    FORWARD> (); }
        iterator& operator -- ()  {  return next<not FORWARD> (); }

        iterator operator+ (size_t nbItems) const  {  return iterator (this->ref_, this->idx_ + K * nbItems, isBegin_);  }
        iterator operator- (size_t nbItems) const  {  return iterator (this->ref_, this->idx_ - K * nbItems, isBegin_);  }

        auto reverse() const {  return  iterator<REFERENCE, !FORWARD> (ref_, idx_-1, true);  }

            auto& cache()       { return cache_.get(ref_); }
      const auto& cache() const { return cache_.get(ref_); }

        iterator_cache<REFERENCE,is_shared_iter_cache> cache_;

        using cache_type = typename decltype(cache_)::cache_type;

        const REFERENCE* ref_   = nullptr;
        size_type        idx_   = 0;
        bool isBegin_ = false;
    };

    // WARNING!!! order of attributes is important here
    ARCH_ALIGN mutable cache_t cache_[CACHE_NB];
    ARCH_ALIGN size_type datablockIdx_  = -1;
    ARCH_ALIGN address_t fillAddress_   =  0;
    ARCH_ALIGN mutable size_type currentBlock_  = -1;
    ARCH_ALIGN mutable MUTEX mutex_;

    cache_t& cache(int idx=0) const { return cache_[idx]; }

    static constexpr int SIZEOF_DATABLOCK = cache_t::SIZEOF_DATABLOCK;

}; // end of vector_view

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
 * The data is indeed split in chunks of MEMORY_SIZE elements and a tree structure is used for keeping
 * track of the order of these data chunks. This design allows to reach some kind of dynamic allocation
 * that is not so easy to support for the UPMEM architecture.
 *
 *  * NOTE: full copy is not allowed (constructor and operator=) but move copy is supported
 */
template<
    typename T,
    typename ALLOCATOR,
    typename MUTEX,
    int MEMORY_SIZE_LOG2,
    int CACHE_NB_LOG2,             // log2 of the number of caches
    bool SHARED_ITER_CACHE,
    int MEMTREE_NBITEMS_PER_BLOCK_LOG2,
    int MAX_MEMORY_LOG2
>
class vector : public vector_view<T,ALLOCATOR,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE>
{
private:

    using parent_t  = vector_view<T,ALLOCATOR,MUTEX,MEMORY_SIZE_LOG2, CACHE_NB_LOG2,SHARED_ITER_CACHE>;
    using cache_t   = typename parent_t::cache_t;

public:

    using memorytree_t = MemoryTree <ALLOCATOR, MEMTREE_NBITEMS_PER_BLOCK_LOG2, MAX_MEMORY_LOG2>;

    static constexpr int SIZEOF_DATABLOCK = parent_t::SIZEOF_DATABLOCK;

    static constexpr int MEMTREE_NBITEMS_PER_BLOCK  = 1 << MEMTREE_NBITEMS_PER_BLOCK_LOG2;
    static constexpr int MAX_MEMORY                 = 1 << MAX_MEMORY_LOG2;

    static constexpr int CACHE_NB                = parent_t::CACHE_NB;
    static constexpr int MEMORY_SIZE             = parent_t::MEMORY_SIZE;
    static constexpr int MEMORY_SIZE_MASK        = parent_t::MEMORY_SIZE_MASK;
    static constexpr int CACHE_NB_ITEMS          = parent_t::CACHE_NB_ITEMS;

    static constexpr uint64_t NBITEMS_MAX = memorytree_t::NBLEAVES_MAX *  CACHE_NB_ITEMS;

    using value_type  = typename parent_t::value_type;
    using size_type   = typename parent_t::size_type;

    using address_t = typename ALLOCATOR::address_t;

    constexpr vector()
    {
        for (size_t i=0; i<CACHE_NB; i++)  { previousBlockIdx_[i] = ~size_type(0); }
    }

    vector (size_type nbitems) {
        // we acquire some memory location (e.g. in MRAM)
        auto address = this->cache(0).acquire(nbitems);
        // and associate this address to the vector.
        parent_t::fill (address, nbitems, sizeof(T));
    }

    // We don't allow to copy a vector.
    vector(const vector& other) = delete;

    vector(vector&& other) :
       parent_t(std::move(other)),
       dirty_               (other.dirty_),
       memoryTree_          (std::move(other.memoryTree_))
    {
        DEBUG_VECTOR ("[vector:: vector (move)]  this: %p  src: %p\n", this, std::addressof(other));

        for (size_t i=0; i<CACHE_NB; i++)  { previousBlockIdx_[i] = other.previousBlockIdx_[i]; }
    }

    vector& operator= (vector&& other)
    {
        DEBUG_VECTOR ("[vector::operator=(move)]  this: %p  src: %p\n", this, std::addressof(other));
        if (this != std::addressof(other))
        {
            parent_t::operator=(std::move(other));
            dirty_            = other.dirty_;
            for (size_t i=0; i<CACHE_NB; i++)  { previousBlockIdx_[i] = other.previousBlockIdx_[i]; }
            memoryTree_       = std::move(other.memoryTree_);
        }
        return *this;
    }

    // For the moment, we avoid to copy assign vectors.
    vector& operator= (const vector& other) = delete;

    constexpr ~vector() = default;

    /** Forbid to get the address. */
    auto* operator& () = delete;

    NOINLINE void push_back (const T& item)
    {
        VERBOSE_VECTOR ("[vector::push_back]  idx before: %d   cachefull: %d  idxCache_: %ld  previousBlockIdx_[idxCache_]: %ld \n", this->datablockIdx_,
            this->cache(idxCache_).isFull(this->datablockIdx_),
            uint64_t(idxCache_),
            uint64_t(previousBlockIdx_[idxCache_])
        );

        // We check that we the current data block is not full
        if (this->cache(idxCache_).isFull(this->datablockIdx_))
        {
            // We flush the vector (cache + memtree).
            flush_block (true);

            // We also remember the block id for a possible call to the operator []
            previousBlockIdx_[idxCache_] = cache_t::getBlockIdx(this->datablockIdx_+1);
        }

        // we increase the index
        this->datablockIdx_++;

        // we add the provided item in the datablock
        this->cache(idxCache_)[this->datablockIdx_] = item;

        this->dirty_ = true;
    }

    /** Appends a new element to the end of the container. */
    template< class... ARGS >
    void emplace_back (ARGS&&... args)
    {
        // no optimization (right now)
        this->push_back ( T(std::forward<ARGS>(args)...) );
    }

    void reset_state()
    {
        vector_view<T,ALLOCATOR,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE>::reset_state();
        idxCache_ = 0;
        dirty_    = false;
        for (size_t i=0; i<CACHE_NB; i++)  { previousBlockIdx_[i] = ~size_type(0); }
    }

    /** Returns the (copied) value at index 'idx'
     * NOTE: this version is const BUT since we need lazy accession, it implies that some
     * attributes need to be mutable
     */

    mutable size_t idxCache_ = 0;

    value_type& operator[] (size_type idx) const
    {
        VERBOSE_VECTOR ("vector::operator[%4ld]  CACHE_MEMORY_SIZE_LOG2: %ld  CACHE_NB: %ld   CACHE_NB_ITEMS: %ld  this: 0x%lx \n",
            (uint64_t)idx,
            (uint64_t)parent_t::CACHE_MEMORY_SIZE_LOG2,
            (uint64_t)parent_t::CACHE_NB,
            (uint64_t)parent_t::CACHE_NB_ITEMS,
            (uint64_t)this
        );

        // We look for a cache that already holds the required information
        size_t look=0;
        size_type blockIdx = cache_t::getBlockIdx(idx);
        for (look=0; look<CACHE_NB; look++)
        {
            if (blockIdx == previousBlockIdx_[look])  { break; }
        }

        if (look==CACHE_NB)
        {
            // We need to get the address where the datablock needs to be retrieved from.
            address_t address = 0;

            // We found no cache with the required index -> we are going to use the next cache.
            // NB: we must take care of the actual number of caches.
            idxCache_ = (idxCache_+1) % parent_t::CACHE_NB;  // should be optimized by the compiler

            previousBlockIdx_[idxCache_] = blockIdx;

            address_t beg=0,end=0;

            if (this->hasBeenFilled())
            {
                // If the vector was filled (ie. call to 'fill'), we know that the blocks are
                // contiguous in memory, so we don't need to use the memory tree then.
                address = this->getFillAddress() + blockIdx*SIZEOF_DATABLOCK;

                beg = this->getFillAddress();
                end = beg + sizeof(T)*this->size();
            }
            else
            {
                // from the memory tree, we get the address of the block to be used
                address = memoryTree_[blockIdx];
            }

            VERBOSE_VECTOR ("vector::operator[]  idxCache_: %ld  current 0x%lx  and next 0x%lx  hasBeenFilled: %ld \n",
                uint64_t(idxCache_),
                uint64_t(this->cache(idxCache_).address()),
                uint64_t(address), uint64_t(this->hasBeenFilled())
            );

            if (address != this->cache(idxCache_).address())
            {
                if (this->cache(idxCache_).address()!=0)  {  this->cache(idxCache_).update();  }

                // we read the new block from the memory.
                // We also provide the memory boundaries of the vector
                this->cache(idxCache_).read (address, beg,end);
            }
        }
        else
        {
            // We found a cache with the required index.
            idxCache_ = look;
        }

        VERBOSE_VECTOR ("vector::operator[%4ld] block: %ld  look: %ld   @cache:  0x%08lx   address: [0x%08lx]   idxCache_: %ld   previousBlockIdx_: [%ld] -----------------> changed [%c]  val: %ld\n",
            (uint64_t)idx,
            (uint64_t)(cache_t::getBlockIdx(idx)),
            uint64_t(look),
            (uint64_t)(&(this->cache(idxCache_))),
            (uint64_t)(this->cache(idxCache_).address()),
            uint64_t(idxCache_),
            (uint64_t)previousBlockIdx_[idxCache_],
            look==CACHE_NB ? 'X' : ' ',
            (uint64_t)this->cache(idxCache_) [idx]
        );

        return this->cache(idxCache_) [idx];
    }

    /** */
    value_type& back () const  {  return (*this) [this->size()-1]; }

    /** */
    size_type max_size () const { return memoryTree_.max_size()*MEMORY_SIZE; }

    template<class REFERENCE, bool FORWARD=true>
    struct iterator : parent_t::template iterator<REFERENCE,FORWARD>
    {
        using parent_type = typename parent_t::template iterator<REFERENCE,FORWARD>;

        iterator (const REFERENCE* ref, size_type idx, typename memorytree_t::leaves_iterator it)
            : parent_type(ref,idx,true), it_(it)
        {
            if (not this->ref_->hasBeenFilled())
            {
                // from the memory tree, we get the address of the block to be used
                // WARNING !!! won't work if idx!=0  (to be improved in MemoryTree)
                this->cache().read (*it_);

                DEBUG_VECTOR ("[vector::iterator]  this: 0x%lx  a_: 0x%lx \n",
                    uint64_t(this),
                    uint64_t(0)
                );
            }

            // the 'else' part should have already been handled by the parent constructor
        }

        iterator (iterator const& other) : iterator(other.ref_, other.idx_, other.it_) {}
        iterator (iterator     && other) : iterator(other.ref_, other.idx_, other.it_) {}

        iterator& operator ++ ()  {
            if (this->ref_->hasBeenFilled()) {
                return static_cast<iterator&>(parent_type::operator++ ());
            }
            // Here, this is quite uneasy to rely on the parent implementation, so we accept a little code duplication.
            if (this->cache().isFull(this->idx_++))  {  this->cache().read (*(++it_)); }
            return *this;
        }

        typename memorytree_t::leaves_iterator it_;
    };

    auto begin()  const
    {
        ((vector*)this)->update_all ();
        return iterator<vector> (this,0,this->memoryTree_.begin());
    }

    auto end  ()  const
    {
        return size_t (this->size());
    }

    static auto dump ()
    {
        DEBUG_VECTOR (
            "[vector]   sizeof: %d  MEMORY_SIZE_LOG2: %d  CACHE_NB_LOG2: %d  CACHE_NB_ITEMS: %d  NBITEMS_MAX: %ld "
            "[memtree]  sizeof: %d  MEMTREE_NBITEMS_PER_BLOCK_LOG2: %d  MAX_MEMORY_LOG2: %d  LEVEL_MAX: %d  NBLEAVES_MAX: %ld  "
            "\n",
            sizeof (vector),       MEMORY_SIZE_LOG2, CACHE_NB_LOG2, CACHE_NB_ITEMS, NBITEMS_MAX,
            sizeof (memorytree_t), MEMTREE_NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2, memorytree_t::LEVEL_MAX,  memorytree_t::NBLEAVES_MAX
        );
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
    }

    NOINLINE auto update() const
    {
        VERBOSE_VECTOR ("[vector::update]  address: 0x%lx   dirty_: %d  size: %ld  CACHE_NB_ITEMS: %ld  check: %d\n",
            uint64_t(this->cache(idxCache_).address()), dirty_, uint64_t(this->size()), uint64_t(CACHE_NB_ITEMS),
            (this->size() % CACHE_NB_ITEMS)==1
        );

        // we save the data block from stack memory to main memory
        if (this->cache(idxCache_).address() != 0)
        {
            this->cache(idxCache_).update();
        }
    }

    NOINLINE auto update_all () const
    {
        for (size_t i=0; i<CACHE_NB; i++)  {
            if (this->cache(i).address() != 0)  {
                this->cache(i).update();
            }
        }
    }

    void flush_block (bool force=false) const
    {
        if (isDirty() or force)
        {
            VERBOSE_VECTOR ("[vector::flush_block] \n");

            this->update();

            // we save the data block from stack memory to main memory
            memoryTree_.insert (this->cache(idxCache_).acquire());

            dirty_ = false;
        }
    }

    void flush_block_all (bool force=true)
    {
        VERBOSE_VECTOR ("[vector::flush_block_all] 0\n");

        for (size_t i=0; i<CACHE_NB; i++)
        {
            if (previousBlockIdx_[i] != size_type(-1))  {   this->cache(i).update();  }
        }
    }

    void fill (address_t address, size_t nbItems, size_t sizeItem)
    {
        // We first call the parent method.
        parent_t::fill (address, nbItems, sizeItem);

        // We count the number of nodes to be added in the memory tree.
        size_t nbfound = (nbItems + (1<<MEMORY_SIZE_LOG2)-1) >> MEMORY_SIZE_LOG2;

        // We process the specific part.
        memoryTree_.insert (address, nbfound, 1<<MEMORY_SIZE_LOG2);

        flush();
    }

    auto& getMemoryTree() const { return memoryTree_; }

    bool isDirty() const
    {
        return dirty_;
    }

    mutable bool       dirty_            = false;
    mutable size_type  previousBlockIdx_[CACHE_NB];
    mutable MemoryTree <ALLOCATOR, MEMTREE_NBITEMS_PER_BLOCK_LOG2, MAX_MEMORY_LOG2> memoryTree_;
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
