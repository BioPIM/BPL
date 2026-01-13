////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifdef DPU  // THIS PART SHOULD BE COMPILED ONLY FOR DPU


#pragma once
////////////////////////////////////////////////////////////////////////////////

#include <bpl/utils/metaprog.hpp>
#include <bpl/utils/serialize.hpp>
#include <bpl/utils/splitter.hpp>
#include <bpl/utils/Range.hpp>
#include <bpl/utils/BufferIterator.hpp>
#include <bpl/utils/vector.hpp>
#include <bpl/utils/tag.hpp>
#include <bpl/arch/dpu/ArchUpmemMRAM.hpp>
#include <bpl/utils/tag.hpp>

#include <mram.h>
#include <algorithm>

#include <span>

extern bpl::MRAM::Allocator<true>  __MRAM_Allocator_lock__;


////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace impl {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

/* The underlying of the following vector implementation is to save items in chunks of N items
 * and keep the MRAM addresses of these chunks into a linked list of pointers tables. Here is
 * an example:
 *
 *   --------
 *   | MRAM |
 *   ---------------------------------------------------------
 *   |                                                       |
 *   |     data chunk    data chunk                          |
 *   |     ----------    ----------                          |
 *   |     | 0 | 35 |    | 0 | 17 |                          |
 *   |     | 1 | 56 |    | 1 | 19 |                          |
 *   |     | 2 | 29 |    | 2 | 21 |     ...                  |
 *   |     | 3 | 22 |    | 3 | 23 |                          |
 *   |     | 4 | 44 |    | 4 | 25 |                          |
 *   |     ----------    ----------                          |
 *   |            ^            ^                             |
 *   |            |            |                             |
 *   |            |            |                             |
 *   |   ---------|-------     |          -----------------  |
 *   |   | ptr  = 0x1234 |     |          | ptr  = 0xA345 |  |
 *   |   | ptr  = 0x3423--------          | ptr  = 0xA789 |  |
 *   |   | ptr  = 0x9843 |                | ptr  = 0xB451 |  |
 *   |   | ptr  = 0xA123 |                | ptr  = null   |  |
 *   |   | next =        | -------------> | next = null   |  |
 *   |   -----------------                -----------------  |
 *   |    pointers table                   pointers table    |
 *   |                                                       |
 *   ---------------------------------------------------------
 */


////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename T, int N>
    struct array
    {
        static constexpr bool parseable = false;

        // IMPORTANT: use __dma_aligned here
        __dma_aligned T val_[N];
        static constexpr int SIZE=N;

        size_t size() const { return N; }

        const T* data() const { return val_; }

        const T& operator[] (uint32_t idx) const  { return val_[idx]; }
              T& operator[] (uint32_t idx)        { return val_[idx]; }

        struct iterator
        {
            const array& ref_;
            __dma_aligned std::size_t idx_ = 0;  // __dma_aligned MANDATORY !!!

            iterator (const array& ref, std::size_t offset) : ref_(ref), idx_(offset)  { }

            iterator operator+ (int32_t offset)
            {
                iterator other (*this);
                other.idx_ += offset;
                return other;
            }

            bool operator != (const iterator& other) const {  return idx_ != other.idx_;  }

            const T& operator* () const   {  return ref_[idx_];  }

            void operator ++ ()  {  ++ idx_;  }
        };

        iterator begin() const {  return iterator(*this,      0); }
        iterator   end() const {  return iterator(*this, size()); }

    };
}

// We specialize the 'get_hash' function for the bpl::impl::array
template<class T,int N>
constexpr auto get_hash (bpl::impl::array<T,N> const& object) noexcept
{
    uint64_t s=0;  for (auto&& x : object)  { s += get_hash(x); }
    return s;
}

////////////////////////////////////////////////////////////////////////////////

class VectorAllocator
{
public:
    using address_t = MRAM::address_t;

    template<int N>
    using address_array_t = __dma_aligned address_t [N];

    static constexpr bool is_freeable = false;

    static address_t get (size_t sizeInBytes)
    {
        return __MRAM_Allocator_lock__.get (sizeInBytes);
    }

    static address_t write (void* src, size_t n)
    {
        return __MRAM_Allocator_lock__.write (src, n);
    };

    static address_t writeAt (void* dest, void* data, size_t sizeInBytes)
    {
        return __MRAM_Allocator_lock__.writeAt (dest, data, sizeInBytes);
    }

    static auto writeAtomic (address_t tgt, address_t const& src)
    {
        return __MRAM_Allocator_lock__.writeAtomic(tgt,src);
    }

    static address_t* read (void* src, void* tgt, size_t n)
    {
        __MRAM_Allocator_lock__.read (src, tgt, n);
        return (address_t*) tgt;
    };

    static void free (address_t a)
    {
        // not really a free, isn't it ?
    }
};

////////////////////////////////////////////////////////////////////////////////

namespace details
{
    struct DefaultConfig
    {
        struct gmutex { void lock() {}  void unlock() {};  };
    };
}

template<class T>  struct gmutex_trait {  using type = typename details::DefaultConfig::gmutex;  };

template<class T>  requires requires { typename T::gmutex; }
struct gmutex_trait<T> {  using type = typename T::gmutex; };

template<typename CONFIG=details::DefaultConfig>
struct ArchUpmemResources
{
    using config_t = CONFIG;

    // Factory that returns a type with a specific configuration.
    // We test whether the CONFIG template parameter provided to ArchUpmemResources is void or not.
    //   - if void,     we use the config provided to the 'factory' template.
    //   - if not void, we use the config provided to the 'ArchUpmemResources' template
    template<typename CFG=details::DefaultConfig> using factory =
        std::conditional_t<
            std::is_same_v <config_t, details::DefaultConfig>,
            ArchUpmemResources<CFG>,
            ArchUpmemResources<config_t>
    >;

    // We define the actual mutex type used for vector synchronization (when 'global' used for instance)
    using gmutex_t = typename gmutex_trait<config_t>::type;

    struct constants_t
    {
        DEFINE_GETTER (VECTOR_MEMORY_SIZE_LOG2);
        DEFINE_GETTER (VECTOR_CACHE_NB_LOG2);
        DEFINE_GETTER (MEMTREE_NBITEMS_PER_BLOCK_LOG2);
        DEFINE_GETTER (MEMTREE_MAX_MEMORY_LOG2);
        DEFINE_GETTER (SWAP_USED);
        DEFINE_GETTER (SHARED_ITER_CACHE);
        DEFINE_GETTER (VECTOR_SERIALIZE_OPTIM);                                                                 \

        static constexpr bool SWAP_USED                     = get_SWAP_USED_v                       <config_t,false>;
        static constexpr int VECTOR_MEMORY_SIZE_LOG2        = get_VECTOR_MEMORY_SIZE_LOG2_v         <config_t,8>;
        static constexpr int VECTOR_CACHE_NB_LOG2           = get_VECTOR_CACHE_NB_LOG2_v            <config_t,SWAP_USED ? 1 : 0>;
        static constexpr int MEMTREE_NBITEMS_PER_BLOCK_LOG2 = get_MEMTREE_NBITEMS_PER_BLOCK_LOG2_v  <config_t,3>;
        static constexpr int MEMTREE_MAX_MEMORY_LOG2        = get_MEMTREE_MAX_MEMORY_LOG2_v         <config_t,8>;
        static constexpr bool SHARED_ITER_CACHE             = get_SHARED_ITER_CACHE_v               <config_t,true>;
        static constexpr bool VECTOR_SERIALIZE_OPTIM        = get_VECTOR_SERIALIZE_OPTIM_v          <config_t,false>;
    };

    template<typename T, typename S> using pair = std::pair<T,S>;

    template<typename T, typename S>  static auto make_pair (T t, S s) { return pair<T,S>(t,s); }

    template<typename T>
    static auto make_reverse_iterator (T&& t) { return t.reverse(); }

    // We currently point to std for some types.

    template<typename T, std::size_t N> using array  = impl::array<T,N>;

    // This type trait is supposed to be provided as second template parameter to 'vector' in order to mimic std::vector API.
    // For the bpl::vector, this type can be used for customization of cache size for instance.
    template<typename T>  struct allocator {
        static const int MEMORY_SIZE_LOG2                = constants_t::VECTOR_MEMORY_SIZE_LOG2 - bpl::Log2<sizeof(T)>::value;
        static const int CACHE_NB_LOG2                   = constants_t::VECTOR_CACHE_NB_LOG2;
        static const bool SHARED_ITER_CACHE              = constants_t::SHARED_ITER_CACHE;
        static const int MEMTREE_NBITEMS_PER_BLOCK_LOG2  = constants_t::MEMTREE_NBITEMS_PER_BLOCK_LOG2;
        static const int MEMTREE_MAX_MEMORY_LOG2         = constants_t::MEMTREE_MAX_MEMORY_LOG2;
    };

    template<
        typename T,
        typename Allocator = allocator<T>,
        int MEMORY_SIZE_LOG2                = Allocator::MEMORY_SIZE_LOG2 + Allocator::CACHE_NB_LOG2,
        int CACHE_NB_LOG2                   = Allocator::CACHE_NB_LOG2,
        bool SHARED_ITER_CACHE              = Allocator::SHARED_ITER_CACHE,
        int MEMTREE_NBITEMS_PER_BLOCK_LOG2  = Allocator::MEMTREE_NBITEMS_PER_BLOCK_LOG2,
        int MEMTREE_MAX_MEMORY_LOG2         = Allocator::MEMTREE_MAX_MEMORY_LOG2
    > using vector  = bpl::vector <
        T,
        VectorAllocator,
        gmutex_t,
        MEMORY_SIZE_LOG2,
        CACHE_NB_LOG2,
        SHARED_ITER_CACHE,
        MEMTREE_NBITEMS_PER_BLOCK_LOG2,
        MEMTREE_MAX_MEMORY_LOG2
    >;


    template<typename T> using span = std::span<T>;

    template<typename T,
        typename Allocator = allocator<T>,
        int MEMORY_SIZE_LOG2                = Allocator::MEMORY_SIZE_LOG2 + Allocator::CACHE_NB_LOG2,
        int CACHE_NB_LOG2                   = Allocator::CACHE_NB_LOG2,
        bool SHARED_ITER_CACHE              = Allocator::SHARED_ITER_CACHE
    >   using vector_view = bpl::vector_view<
        T,
        VectorAllocator,
        gmutex_t,
        MEMORY_SIZE_LOG2,
        CACHE_NB_LOG2,
        SHARED_ITER_CACHE
    >;

                                struct string {};

                                using size_t = std::size_t;

    template<typename ...Ts>    using tuple = std::tuple<Ts...>;


    template<typename ...Ts>
    static auto make_tuple (Ts... t) { return std::make_tuple(t...); }

    template<std::size_t I, class... Types >
    static auto get(const tuple<Types...>& t ) noexcept  { return std::get<I>(t); }

    template<std::size_t I, class... Types >
    static auto get(tuple<Types...>& t ) noexcept  { return std::get<I>(t); }

    template <typename ...Args>
    static void info (const char * format, Args ...args)  {  printf(format, args...);  }

    static auto round (int n)  {  return bpl::roundUp<8>(n); }

    template<typename T> using once   = bpl::once<T>;
    template<typename T> using global = bpl::global<T>;

    struct mutex
    {
    public:
        using type     = uint8_t;
        using handle_t = type*;

        constexpr mutex () : handle_(0) {}

        constexpr mutex (type& value)   : handle_(&value) {}

        void lock  () { mutex_lock   (handle()); /*printf ("lock\n");  */ }
        void unlock() { mutex_unlock (handle()); /*printf ("unlock\n");*/ }

    private:
        handle_t handle_;
        handle_t handle() { return handle_; }
    };

    template<typename M> struct lock_guard
    {
        M& mut_;
        explicit lock_guard (M& mut) : mut_(mut)  { mut_.lock(); }
        ~lock_guard ()  { mut_.unlock(); }
    };

    template<int VALUE>  struct ErrorOnlyOneCacheForSwapUsage     {  auto operator() () {}       };
    template<>           struct ErrorOnlyOneCacheForSwapUsage<0>  { /* we remove the operator. */};

    template <class T> static void swap ( T& a, T& b )
    {
        ErrorOnlyOneCacheForSwapUsage<constants_t::VECTOR_CACHE_NB_LOG2>() ();
        T c(a); a=b; b=c;
    }

    template <class T> static T max ( T& a, T& b ) { return a > b ? a : b; }
    template <class T> static T min ( T& a, T& b ) { return a < b ? a : b; }

}; // end of ArchUpmemResources

////////////////////////////////////////////////////////////////////////////////
template<typename T>  struct is_vector : std::false_type {};

template<typename T, typename MUTEX,
    int MEMORY_SIZE_LOG2,
    int CACHE_NB_LOG2,
    bool SHARED_ITER_CACHE,
    int MEMTREE_NBITEMS_PER_BLOCK_LOG2,
    int MAX_MEMORY_LOG2
>
struct is_vector<bpl::vector<T,VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE,MEMTREE_NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2>>
    : std::true_type {};

template<typename T, typename MUTEX,
    int MEMORY_SIZE_LOG2,
    int CACHE_NB_LOG2,
    bool SHARED_ITER_CACHE
>
struct is_vector<bpl::vector_view<T,VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE>>
    : std::true_type {};

// We need a specialization for tag 'global' => we don't count a vector if encaspulated by such a tag.
template<typename T> struct is_vector<bpl::global<T>> : std::false_type {};

// We need a specialization for tag 'once'
template<typename T> struct is_vector<bpl::once  <T>> : is_vector<typename bpl::once  <T>::type> {};

template<typename T, typename MUTEX,
    int MEMORY_SIZE_LOG2,
    int CACHE_NB_LOG2,
    bool SHARED_ITER_CACHE,
    int MEMTREE_NBITEMS_PER_BLOCK_LOG2,
    int MAX_MEMORY_LOG2
>
void reset_state (bpl::vector<T,VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE,MEMTREE_NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2>& x)
{
    x.reset_state();
}

template<typename T, typename MUTEX,
    int MEMORY_SIZE_LOG2,
    int CACHE_NB_LOG2,
    bool SHARED_ITER_CACHE
>
void reset_state (bpl::vector_view<T,VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE>& x)
{
    x.reset_state();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////

namespace bpl {

    struct Config4Global
    {
        // synchro when using random access with operator[]
        using gmutex  = ArchUpmemResources<>::mutex;

        // no synchro but specific cache for each tasklet when using iterator objects
        static constexpr bool SHARED_ITER_CACHE = false;
    };

    template<typename CONFIG>
    class BufferIterator<bpl::ArchUpmemResources<CONFIG>>
    {
    public:
        using type_t    = char;
        using pointer_t = type_t*;
        using size_type = uint32_t;
        using address_t = bpl::MRAM::address_t;

        BufferIterator (__mram_ptr void* buf_MRAM, pointer_t cache_WRAM, size_type size)
            : source_MRAM_(buf_MRAM), start_WRAM_(cache_WRAM), size_(size)
        {
            readNext (0);
        }

        template<typename T> T& object (size_type size)
        {
            debug ("object");
            // size_type dist = loop_WRAM_ - start_WRAM_;
            // if (dist + size > size_)  { readNext(dist);  }
            return * ((T*)loop_WRAM_);
        }

        void advance (size_type n)
        {
            debug ("advance");
            // We update both WRAM and MRAM pointers
            loop_WRAM_  += n;

            size_type dist = loop_WRAM_ - start_WRAM_;
            if (dist + n > size_)  { readNext(dist);  }

            source_MRAM_ = (__mram_ptr void*) ( (pointer_t)source_MRAM_ + n);
        }

        void memcpy (pointer_t target, size_type size)
        {
            debug ("memcpy");

            size_t step = std::min (size, size_t(2048));

            while (size > 0)
            {
                __MRAM_Allocator_lock__.reader_ ((pointer_t)source_MRAM_, (void*)target, step);
                source_MRAM_ = (__mram_ptr void*) ( (pointer_t)source_MRAM_ + step);

                target += step;
                size   -= step;
                step    = std::min (size, size_t(2048));
            }

            // We force the reading of the cache from the new current position in MRAM
            readNext(0);
        }

        void read (void* target, size_type size)
        {
            debug ("read");
            __MRAM_Allocator_lock__.reader_ ((pointer_t)source_MRAM_, (void*)target, size);
            readNext(size);
        }

        template<typename T>
        void initialize (T& target, size_type idx)
        {
        }

        auto pos() {

            //size_type dist = loop_ - cache_;
            return (void*) (source_MRAM_) ;
        }

//    private:

        void debug (const char* msg)
        {
            if (false and me()==0)
            {
                printf ("[%-20s]   size_: %ld  loop_WRAM_: %ld  source_MRAM_: %ld   dist: %ld \n", msg,
                   (uint64_t)size_, (uint64_t)loop_WRAM_, (uint64_t)source_MRAM_, (uint64_t) (loop_WRAM_ - start_WRAM_)
                );
            }
        }

        void readNext (size_type offset)
        {
            debug ("readNext BEGIN");

            // We point to the next position to be read in MRAM
            source_MRAM_ = (__mram_ptr void*) ((pointer_t)source_MRAM_+offset);

            // We read some bytes from MRAM to WRAM
            __MRAM_Allocator_lock__.reader_ ((pointer_t)source_MRAM_, (void*)start_WRAM_, size_);

            // We reset the cursor reading the WRAM
            loop_WRAM_ = start_WRAM_;

            debug ("readNext END");
        }

        __mram_ptr void* source_MRAM_;
        __dma_aligned pointer_t start_WRAM_;
        __dma_aligned pointer_t loop_WRAM_ = start_WRAM_;
        size_type size_;
    };


    // We provide a specialization for 'global' whose type is a bpl vector_view.
    // In such a case, we make sure that the iterator cache is an external one,
    // so we don't have issue by sharing the same iter cache between tasklet
    template<typename T, typename MUTEX, int DATABLOCK_SIZE_LOG2, int  CACHE_NB_LOG2, bool SHARED_ITER_CACHE>
    struct global_converter <
        bpl::vector_view <T,bpl::VectorAllocator,MUTEX,DATABLOCK_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE>
    >
    {
        using mutex_t = ArchUpmemResources<>::mutex;

        // We force to use an external cache for vector_view iteration.
        using type = bpl::vector_view <T, bpl::VectorAllocator, mutex_t, DATABLOCK_SIZE_LOG2, CACHE_NB_LOG2, false>;
    };

    // We provide a specialization for 'global' whose type is a bpl vector -> transform it into a vector_view
    template<
        typename T,
        typename MUTEX,
        int MEMORY_SIZE_LOG2,
        int CACHE_NB_LOG2,             // log2 of the number of caches
        bool SHARED_ITER_CACHE,
        int MEMTREE_NBITEMS_PER_BLOCK_LOG2,
        int MAX_MEMORY_LOG2
    >
    struct global_converter <
        bpl::vector <T,bpl::VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE,MEMTREE_NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2>
    >
    {
        using mutex_t = ArchUpmemResources<>::mutex;

        // We force to use an external cache for vector_view iteration.
        using type = bpl::vector_view <T, bpl::VectorAllocator, mutex_t, MEMORY_SIZE_LOG2, CACHE_NB_LOG2, false>;
    };

    template<template<typename> typename T, typename CFG>
    struct global_converter < T<ArchUpmemResources<CFG>> >
    {
        // We want to replace the provided ARCH by another architecture.
        // In particular, we want to use mutex protection, which can be done by providing a specific configuration.
        using type = T <ArchUpmemResources<Config4Global>>;

        // By the way, Config4Global has to be defined outside this struct, otherwise compiler may do strange things.
        // (should be investigated, maybe a compiler bug...)
    };
};

////////////////////////////////////////////////////////////////////////////////

namespace bpl {

    template<typename T>
    struct is_parseable<std::span<T>> : std::false_type {};

    template<typename T,
        typename MUTEX,
        int MEMORY_SIZE_LOG2,
        int CACHE_NB_LOG2,
        bool SHARED_ITER_CACHE,
        int MEMTREE_NBITEMS_PER_BLOCK_LOG2,
        int MAX_MEMORY_LOG2
    >
    struct serializable<bpl::vector<T,bpl::VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE,MEMTREE_NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2>> : std::true_type
    {
        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE, typename FCT>
        static auto iterate (bool transient, int depth, const TYPE& t, FCT fct, void* context)
        {
            using serial_size_t = typename Serialize<ARCH,BUFITER,ROUNDUP>::serial_size_t;

            if constexpr(ARCH::constants_t::VECTOR_SERIALIZE_OPTIM)  { if (t.hasBeenFilled())  { return; } }

            // We serialize the size of the vector.
            serial_size_t n = t.size();
            Serialize<ARCH,BUFITER,ROUNDUP>::iterate (true, depth+1, n, fct);

            if (context==nullptr)
            {
                // We serialize each part of the vector since the blocks are not contiguous in MRAM.
                // A better implementation should avoid this data copy...
                for (const auto& x : t)
                {
                    Serialize<ARCH,BUFITER,ROUNDUP>::iterate (false, depth+1, x, fct, context);
                }
            }
#if 0  // NOT USED RIGHT NOW
            else
            {
                // We get the memory size of the cache of the vector.
                constexpr int SIZEOF = std::remove_reference_t<decltype(t)>::SIZEOF_DATABLOCK;
                static_assert (SIZEOF>=8);

                // We rely on the memory tree for the serialization.
                // The idea is to be able to serialize a full block instead of item after item.

                using memtree_t = std::remove_reference_t<decltype (t.getMemoryTree())>;
                const int nbread = memtree_t::BLOCK_SIZEOF;

                for (auto address : t.getMemoryTree())
                {
                    // We copy the block from the MRAM to the provided WRAM cache (as 'context')
                    __MRAM_Allocator_lock__.reader_ ((void*)address, (void*)context, nbread);

                    fct (false, depth+1, context, nbread, roundUp<ROUNDUP> (nbread));
                }
            }
#endif
        }

        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE>
        static auto restore (BUFITER& it, TYPE& result)
        {
            using serial_size_t = typename Serialize<ARCH,BUFITER,ROUNDUP>::serial_size_t;

            bool status = true;

            serial_size_t n=0;
            it.read (&n, roundUp<ROUNDUP> (sizeof(n)) );

            if (n>0)
            {
                // We have already the data in MRAM, but we need to build a MemoryTree whose nodes point to the different data blocks in MRAM.
                result.fill ((bpl::VectorAllocator::address_t) it.pos(), n, sizeof(T));

                // We advance the buf iterator.
                it.readNext (roundUp<ROUNDUP>(n*sizeof(T)));
            }
            else
            {
                // in case the vector is empty, we still provide some data in order not to have an nullptr.
                auto [dummy, N] = Serialize<ARCH,BUFITER,ROUNDUP>::getDummyBuffer();
                it.readNext (N);
            }

#ifdef WITH_SERIALIZATION_HASH_CHECK
            uint64_t check1 = 0;
            it.read (&check1, roundUp<ROUNDUP> (sizeof(check1)) );

            uint64_t check2 = 0;
            for (auto const& x : result)  { check2 += get_hash(x); }

            status = check1 == check2;
#endif
            return status;
        }
    };


    template<typename T,typename MUTEX, int DATABLOCK_SIZE_LOG2,int CACHE_NB_LOG2, bool SHARED_ITER_CACHE>
    struct serializable<bpl::vector_view<T,bpl::VectorAllocator,MUTEX,DATABLOCK_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE>>
    {
        static constexpr int value = true;

        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE, typename FCT>
        static auto iterate (bool transient, int depth, const TYPE& t, FCT fct, void* context=nullptr)
        {
            using serial_size_t = typename Serialize<ARCH,BUFITER,ROUNDUP>::serial_size_t;

            DEBUG_SERIALIZATION ("iterate UPMEM vector_view\n");

            // We serialize the size of the vector.
            serial_size_t n = t.size();
            Serialize<ARCH,BUFITER,ROUNDUP>::iterate (true, depth+1, n, fct, context);

            // We serialize each part of the vector since the blocks are not contiguous in MRAM.
            // A better implementation should avoid this data copy...
            for (const auto& x : *(TYPE*)(&t))
            {
                Serialize<ARCH,BUFITER,ROUNDUP>::iterate (false, depth+1, x, fct);
            }
        }

        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE>
        static auto restore (BUFITER& it, TYPE& result)
        {
            using serial_size_t = typename Serialize<ARCH,BUFITER,ROUNDUP>::serial_size_t;

            bool status = true;

            serial_size_t n=0;
            it.read (&n, roundUp<ROUNDUP> (sizeof(n)) );

            if (n>0)
            {
                // We have already the data in MRAM, but we need to build a MemoryTree whose nodes point to the different data blocks in MRAM.
                result.fill ((bpl::VectorAllocator::address_t) it.pos(), n, sizeof(T));

                // We advance the buf iterator.
                it.readNext (roundUp<ROUNDUP>(n*sizeof(T)));
            }
            else
            {
                // in case the vector is empty, we still provide some data in order not to have an nullptr.
                auto [dummy, N] = Serialize<ARCH,BUFITER,ROUNDUP>::getDummyBuffer();
                it.readNext (N);
            }

#ifdef WITH_SERIALIZATION_HASH_CHECK
            uint64_t check1 = 0;
            it.read (&check1, roundUp<ROUNDUP> (sizeof(check1)) );

            uint64_t check2 = 0;
            for (auto const& x : result)  { check2 += get_hash(x); }

            status = check1 == check2;
#endif
            return status;
        }
    };

    template<typename T, int N>
    struct serializable<bpl::ArchUpmemResources<>::array<T,N>>
    {
        static constexpr int value = true;

        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE, typename FCT>
        static auto iterate (bool transient, int depth, const TYPE& t, FCT fct, void* context=nullptr)
        {
            fct (transient, depth, (void*)&t, sizeof(TYPE),  roundUp<ROUNDUP>(sizeof(TYPE)));
        }

        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE>
        static auto restore (BUFITER& it, TYPE& result)
        {
            bool status = true;

            // We copy the array from the MRAM to the WRAM (where we suppose to have the required room)
            it.memcpy ((char*)&result, roundUp<ROUNDUP>(N*sizeof(T)));

            return status;
        }
    };
};

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

template<typename T>
auto getSplitRange (std::size_t length, std::size_t idx, std::size_t total)
{
    auto range   = std::make_pair (0, length);
    auto [i0,i1] = SplitOperator<decltype(range)>::split (std::make_pair (0, length), idx, total);

#if 0
    // UPDATE: we remove this static check since we can now handle address_t on uint32_t.
    // --> maybe we should now check whether the computed address is a multiple of 4.

    // WARNING!!! we must be sure that the computed address is a multiple of 8
    // Since we suppose that the initial address fulfills this requirement,
    // we hence must ensure that i0*sizeof(T) is a modulo of 8

    static_assert ( sizeof(T)<=8 ?
        ( (8 % sizeof(T)) == 0) :
        ( (sizeof(T) % 8) == 0),
        "alignment requirements not fulfilled for split."
    );
#endif

    constexpr int DIV = sizeof(T)<=8 ?  (8 / sizeof(T)) :  1;

    if ((i0*sizeof(T) % 8)!=0 )  {  i0 = bpl::roundUp<DIV>(i0);  }
    if ((i1*sizeof(T) % 8)!=0 )  {  i1 = bpl::roundUp<DIV>(i1);  }

    // NB: we allow only for the last part to be not aligned
    if (idx+1 == total)  {  i1 = length;  }

    return std::make_pair (i0,i1);
}

//////////////////////////////////////////////////////////////////////////////////////////
template<typename T, typename MUTEX, 
    int MEMORY_SIZE_LOG2,
    int CACHE_NB_LOG2,
    bool SHARED_ITER_CACHE,
    int MEMTREE_NBITEMS_PER_BLOCK_LOG2,
    int MAX_MEMORY_LOG2
>
struct SplitOperator<bpl::vector<T,bpl::VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE,MEMTREE_NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2>>
{
    static auto split (const bpl::vector<T,bpl::VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE,MEMTREE_NBITEMS_PER_BLOCK_LOG2,MAX_MEMORY_LOG2>& x, std::size_t idx, std::size_t total)
    {
        using vector_t = std::decay_t<decltype(x)>;

        auto [i0,i1] = getSplitRange<T> (x.size(), idx, total);

        vector_t result;

        if (not x.hasBeenFilled())
        {
            // NOT OPTIMAL AT ALL !!! The split process could be done during unserialization.
            // Even worse here: we read the full vector for each tasklet...
            int i=0;
            for (const auto& item : ( vector_t&) x)
            {
                if (int(i0)<=i and i<int(i1))  { result.push_back (item); }
                i++;
            }

            result.flush();
        }
        else
        {
            bpl::VectorAllocator::address_t a = x.getFillAddress() + i0*sizeof(T);
            result.fill (a, i1-i0, sizeof(T));
        }

        return result;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////
template<typename T,typename MUTEX, int MEMORY_SIZE_LOG2,int CACHE_NB_LOG2, bool SHARED_ITER_CACHE>
struct SplitOperator<bpl::vector_view<T,bpl::VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE>>
{
    using type = bpl::vector_view<T,bpl::VectorAllocator,MUTEX,MEMORY_SIZE_LOG2,CACHE_NB_LOG2,SHARED_ITER_CACHE>;

    static auto split (const type& x, std::size_t idx, std::size_t total)
    {
        auto [i0,i1] = getSplitRange<T> (x.size(), idx, total);

        bpl::VectorAllocator::address_t a = x.getFillAddress() + i0*sizeof(T);

        type result;
        result.fill (a, i1-i0, sizeof(T));

        return result;
    }
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////


namespace bpl  {

template<int MUTEX_NB, typename CONFIG>
struct Task <bpl::ArchUpmemResources<CONFIG>,MUTEX_NB> : TaskBase <Task<bpl::ArchUpmemResources<CONFIG>,MUTEX_NB>>
{
    using parent_t = TaskBase<Task<bpl::ArchUpmemResources<CONFIG>,MUTEX_NB>>;

    using tuid_t   = typename parent_t::tuid_t;
    using cycles_t = typename parent_t::cycles_t;

    cycles_t nbcycles() const  { return perfcounter_get();  }

    void notify (size_t current, size_t total) {}

    bool match_tuid (tuid_t value) const  { return (value%NR_TASKLETS) == (this->tuid()%NR_TASKLETS); }

//    static auto& get_mutex (int idx)
//    {
//        static typename bpl::ArchUpmemResources::mutex  mutexes[MUTEX_NB];
//        return  mutexes[idx];
//    }

};

} // namespaces

#endif  // #ifdef DPU
