////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>
#include <bpl/utils/vector.hpp>

extern "C"
{
int check_stack();
}

#ifdef DPU
extern bpl::MRAM::Allocator<true> __MRAM_Allocator_lock__;
#endif

////////////////////////////////////////////////////////////////////////////////

class MyAllocator
{
public:
    using address_t = uint64_t;

    template<int N>  using address_array_t =
#ifdef DPU
            __dma_aligned
#endif
            address_t [N];

    static constexpr bool is_freeable = false;

    static address_t get (size_t sizeInBytes)
    {
        address_t result = {};
#ifdef DPU
        result = __MRAM_Allocator_lock__.get (sizeInBytes);
#endif
        return result;
    }

    static address_t writeAt (void* dest, void* data, size_t sizeInBytes)
    {
#ifdef DPU
        __MRAM_Allocator_lock__.writeAt (dest,data,sizeInBytes);
#endif
        return (address_t) dest;
    }

    static auto writeAtomic (address_t tgt, address_t const& src)
    {
#ifdef DPU
        __MRAM_Allocator_lock__.writeAtomic (tgt, src);
#endif
    }

    static address_t write (void* src, size_t n)
    {
        address_t result = 0;
#ifdef DPU
        result = __MRAM_Allocator_lock__.write (src, n);
#endif
        return result;
    };

    static address_t* read (void* src, void* tgt, size_t n)
    {

#ifdef DPU
        __MRAM_Allocator_lock__.read (src, tgt, n);
#endif
        return (address_t*) tgt;
    };


    static void free (address_t a)
    {
        // not really a free, isn't it ?
    }

    static void reset()
    {
#ifdef DPU
        __MRAM_Allocator_lock__.reset();
#endif
    }

    static void reset_stats()
    {
    }
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Vector3 : bpl::Task<ARCH>
{
    USING(ARCH);

    struct MutexNull { void lock() {}  void unlock() {}; };

    using type_t   = uint32_t;

    using vector_t = bpl::vector<type_t,MyAllocator,MutexNull,8,1,3,9>;

    uint64_t operator() (size_t nbtasks, uint32_t nbitems)
    {
        uint64_t checksum = 0;

        if (this->tuid()>=nbtasks)  { return checksum; }

        vector_t v;

        for (type_t i=1; i<=nbitems; i++)  { v.push_back (i); }

        for (auto x : v)   {  checksum += x; }

        return checksum;
    }
};
