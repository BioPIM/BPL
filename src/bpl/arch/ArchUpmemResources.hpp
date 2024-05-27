////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifdef DPU  // THIS PART SHOULD BE COMPILED ONLY FOR DPU

#ifndef __BPL_ARCH_UPMEM_RESOURCES__
#define __BPL_ARCH_UPMEM_RESOURCES__
////////////////////////////////////////////////////////////////////////////////

#include <bpl/utils/metaprog.hpp>
#include <bpl/utils/serialize.hpp>
#include <bpl/utils/split.hpp>
#include <bpl/utils/Range.hpp>
#include <bpl/utils/BufferIterator.hpp>
#include <bpl/utils/vector.hpp>
#include <bpl/utils/tag.hpp>
#include <bpl/arch/dpu/ArchUpmemMRAM.hpp>

#include <mram.h>
#include <algorithm>

extern bpl::arch::MRAM::Allocator<true>  __MRAM_Allocator_lock__;


////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace arch {

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
        // IMPORTANT: use __dma_aligned here
        __dma_aligned T val_[N];
        static constexpr int SIZE=N;

        size_t size() const { return N; }

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

////////////////////////////////////////////////////////////////////////////////

class VectorAllocator
{
public:
    using address_t = MRAM::address_t;

    static constexpr bool is_freeable = false;

    static address_t get (size_t sizeInBytes)
    {
        return __MRAM_Allocator_lock__.get (sizeInBytes);
    }

    static address_t write (void* src, size_t n)
    {
        return __MRAM_Allocator_lock__.write (src, n);
    };

    static address_t writeAt (address_t dest, void* data, size_t sizeInBytes)
    {
        return __MRAM_Allocator_lock__.writeAt (dest, data, sizeInBytes);
    }

    static address_t* read (address_t src, void* tgt, size_t n)
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

////////////////////////////////////////////////////////////////////////////////
struct ArchUpmemResources
{
    template<typename T, typename S> using pair = std::pair<T,S>;

    template<typename T, typename S>  static auto make_pair (T t, S s) { return pair<T,S>(t,s); }

    // We currently point to std for some types.

    template<typename T, std::size_t N> using array  = impl::array<T,N>;

    template<typename T,
        int DATABLOCK_SIZE_LOG2             = 9 - core::Log2<sizeof(T)>::value,
        int MEMTREE_NBITEMS_PER_BLOCK_LOG2  = 3,
        int MAX_MEMORY_LOG2                 = 8
    > using vector  = bpl::core::vector<T,VectorAllocator, DATABLOCK_SIZE_LOG2, MEMTREE_NBITEMS_PER_BLOCK_LOG2, MAX_MEMORY_LOG2>;


    template<typename T> using span = std::span<T>;

    template<typename T,
        int DATABLOCK_SIZE_LOG2             = 9 - core::Log2<sizeof(T)>::value
    >                using vector_view = bpl::core::vector_view<T,VectorAllocator, DATABLOCK_SIZE_LOG2>;

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

    static auto round (int n)  {  return core::roundUp<8>(n); }

    template<typename T> using once   = bpl::core::once<T>;
    template<typename T> using global = bpl::core::global<T>;

    struct mutex
    {
    public:
        using type     = uint8_t;
        using handle_t = type*;

        constexpr mutex () : handle_(0) {}

        constexpr mutex (type& value)   : handle_(&value) {}

        void lock  () { mutex_lock   (handle()); }
        void unlock() { mutex_unlock (handle()); }

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

    template <class T> static void swap ( T& a, T& b ) {  T c(a); a=b; b=c; }
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

namespace bpl { namespace core {

    template<>
    class BufferIterator<bpl::arch::ArchUpmemResources>
    {
    public:
        using type_t    = char;
        using pointer_t = type_t*;
        using size_type = uint32_t;

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

        void memcpy (void* target, size_type size)
        {
            debug ("memcpy");
            size_type dist = loop_WRAM_ - start_WRAM_;
            target = (void*) ((uint8_t*)source_MRAM_ + dist) ;
            advance (size);
        }

        void read (void* target, size_type size)
        {
            debug ("read");
            mram_read  (source_MRAM_, target, size);
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
            mram_read  (source_MRAM_, start_WRAM_, size_);

            // We reset the cursor reading the WRAM
            loop_WRAM_ = start_WRAM_;

            debug ("readNext END");
        }

        __mram_ptr void* source_MRAM_;
        __dma_aligned pointer_t start_WRAM_;
        __dma_aligned pointer_t loop_WRAM_ = start_WRAM_;
        size_type size_;
    };
} };

////////////////////////////////////////////////////////////////////////////////
#endif // __BPL_ARCH_UPMEM_RESOURCES__

namespace bpl { namespace core {

    template<typename T>
    struct serializable<arch::ArchUpmemResources::vector<T>> : std::true_type
    {
        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE, typename FCT>
        static auto iterate (int depth, const TYPE& t, FCT fct)
        {
            // We serialize the size of the vector.
            uint64_t n = t.size();
            Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, n, fct);

            // We serialize each part of the vector since the blocks are not contiguous in MRAM.
            // A better implementation should avoid this data copy...
            for (const auto& x : *(TYPE*)(&t))
            {
                Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, x, fct);
            }
        }

        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE>
        static auto restore (BUFITER& it, TYPE& result)
        {
            size_t n=0;
            it.read (&n, roundUp<ROUNDUP> (sizeof(n)) );

            // We have already the data in MRAM, but we need to build a MemoryTree whose nodes point to the different data blocks in MRAM.
            result.fill ((arch::VectorAllocator::address_t) it.pos(), n, sizeof(T));

            // We advance the buf iterator.
            it.readNext (roundUp<ROUNDUP>(n*sizeof(T)));
        }
    };


    template<typename T>
    struct serializable<arch::ArchUpmemResources::vector_view<T>>
    {
        static constexpr int value = true;

        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE, typename FCT>
        static auto iterate (int depth, const TYPE& t, FCT fct)
        {
            // We serialize the size of the vector.
            uint64_t n = t.size();
            Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, n, fct);

            // We serialize each part of the vector since the blocks are not contiguous in MRAM.
            // A better implementation should avoid this data copy...
            for (const auto& x : *(TYPE*)(&t))
            {
                Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, x, fct);
            }
        }

        template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE>
        static auto restore (BUFITER& it, TYPE& result)
        {
            size_t n=0;
            it.read (&n, roundUp<ROUNDUP> (sizeof(n)) );

            // We have already the data in MRAM, but we need to build a MemoryTree whose nodes point to the different data blocks in MRAM.
            result.fill ((arch::VectorAllocator::address_t) it.pos(), n, sizeof(T));

            // We advance the buf iterator.
            it.readNext (roundUp<ROUNDUP>(n*sizeof(T)));
        }
    };

}};

//////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
auto getSplitRange (std::size_t length, std::size_t idx, std::size_t total)
{
    auto range   = std::make_pair (0, length);
    auto [i0,i1] = SplitOperator<decltype(range)>::split (std::make_pair (0, length), idx, total);

    // WARNING!!! we must be sure that the computed address is a modulo of 8
    // Since we suppose that the initial address fulfills this requirement,
    // we hence must ensure that i0*sizeof(T) is a modulo of 8

    static_assert ( sizeof(T)<=8 ?
        ( (8 % sizeof(T)) == 0) :
        ( (sizeof(T) % 8) == 0),
        "alignment requirements not fulfilled for split."
    );

    constexpr int DIV = sizeof(T)<=8 ?  (8 / sizeof(T)) :  1;

    if ((i0*sizeof(T) % 8)!=0 )  {  i0 = bpl::core::roundUp<DIV>(i0);  }
    if ((i1*sizeof(T) % 8)!=0 )  {  i1 = bpl::core::roundUp<DIV>(i1);  }

   return std::make_pair (i0,i1);
}

//////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct SplitOperator<bpl::arch::ArchUpmemResources::vector<T>>
{
    static auto split (const bpl::arch::ArchUpmemResources::vector<T>& x, std::size_t idx, std::size_t total)
    {
        auto [i0,i1] = getSplitRange<T> (x.size(), idx, total);

        bpl::arch::ArchUpmemResources::vector<T> result;

//        printf ("[DPU] split:  filled: %d  idx: %2d  total: %2d =>  [%3d,%3d]  %d \n",
//            x.hasBeenFilled(), idx, total, i0, i1, (i0*sizeof(T) % 8)==0
//        );

        if (not x.hasBeenFilled())
        {
            // NOT OPTIMAL AT ALL !!! The split process could be done during unserialization.
            // Even worse here: we read the full vector for each tasklet...
            size_t i=0;
            for (const auto& item : ( bpl::arch::ArchUpmemResources::vector<T>&) x)
            {
                if (i0<=i and i<i1)  { result.push_back (item); }
                i++;
            }

            result.flush();
        }
        else
        {
            bpl::arch::VectorAllocator::address_t a = x.getFillAddress() + i0*sizeof(T);
            result.fill (a, i1-i0, sizeof(T));
        }

        return result;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct SplitOperator<bpl::arch::ArchUpmemResources::vector_view<T>>
{
    static auto split (const bpl::arch::ArchUpmemResources::vector_view<T>& x, std::size_t idx, std::size_t total)
    {
        auto [i0,i1] = getSplitRange<T> (x.size(), idx, total);

        bpl::arch::VectorAllocator::address_t a = x.getFillAddress() + i0*sizeof(T);

        bpl::arch::ArchUpmemResources::vector_view<T> result;
        result.fill (a, i1-i0, sizeof(T));

        return result;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct bpl::core::surrogate <bpl::arch::ArchUpmemResources::vector<T>>
{
    using type = typename bpl::arch::ArchUpmemResources::vector_view <T>;
};


//////////////////////////////////////////////////////////////////////////////////////////
//template<typename T, typename S>
//struct SplitOperator<bpl::arch::ArchUpmemResources::pair<T,S>>
//{
//    static auto split (bpl::arch::ArchUpmemResources::pair<T,S> r, std::size_t idx, std::size_t total)
//    {
//        if (total==0)  { return r; }
//
//        auto diff = r.second - r.first;
//
//        size_t i0 = diff * (idx+0) / total;
//        size_t i1 = diff * (idx+1) / total;
//
//        printf ("UPMEM SPLIT<pair>:  [%ld,%ld]  idx=%d  total=%d  diff=%ld  [i0,i1]=[%ld,%ld] \n",
//            r.first, r.second, idx, total, diff, i0, i1
//        );
//
//        return std::pair<T,S> (r.first + i0, r.first + i1);
//    }
//};

namespace bpl  { namespace core {

template<int MUTEX_NB>
struct Task <arch::ArchUpmemResources,MUTEX_NB> : TaskBase <Task<arch::ArchUpmemResources,MUTEX_NB>>
{
    using parent_t = TaskBase<Task<arch::ArchUpmemResources,MUTEX_NB>>;
    using tuid_t   = typename parent_t::tuid_t;
    using cycles_t = typename parent_t::cycles_t;

    cycles_t nbcycles() const  { return perfcounter_get();  }

//    static auto& get_mutex (int idx)
//    {
//        static typename arch::ArchUpmemResources::mutex  mutexes[MUTEX_NB];
//        return  mutexes[idx];
//    }

};

}} // namespaces

#endif  // #ifdef DPU
