////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

// We define our custom allocator.
// Through inheritance, we set it as the default one defined for ARCH
// (which will be std::allocator in case of multicore arch)
template<typename ARCH,int NBITEMS_LOG2>
struct MyCustomAllocator : bpl::DefaultAllocator<ARCH> {};

#ifdef DPU
// Template specialization for UPMEM for our custom allocator.
template<typename Config,int NBITEMS_LOG2>
struct MyCustomAllocator<bpl::ArchUpmemResources<Config>,NBITEMS_LOG2>  {
    // Defining all the required constants for bpl::vector
    template<typename T>
    struct type {
        static const int  MEMORY_SIZE_LOG2               = NBITEMS_LOG2 + bpl::Log2<sizeof(T)>::value;
        static const int  CACHE_NB_LOG2                  = 0;
        static const bool SHARED_ITER_CACHE              = true;
        static const int  MEMTREE_MAX_MEMORY_LOG2        = 8;
        static const int  MEMTREE_NBITEMS_PER_BLOCK_LOG2 = 3;
    };
};
#endif

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorCustomAllocator : bpl::Task<ARCH>
{
    USING(ARCH);

    // We define a type shortcut for our custom allocator.
    // If we use UPMEM, the template specialization will be used instead of the
    // generic implementation (std::allocator in case of multicore)
    template<typename T,int NBITEMS_LOG2>
    using allocator_t = typename MyCustomAllocator<ARCH,NBITEMS_LOG2>::template type<T>;

    template<typename T,int NBITEMS_LOG2=1>   // at least two items in the cache
    using myvector_t = vector<T,allocator_t<T,NBITEMS_LOG2>>;

#ifdef DPU
    // Some checks only for DPU
    static_assert (myvector_t<int32_t,1>::CACHE_NB_ITEMS ==  2);
    static_assert (myvector_t<int32_t,2>::CACHE_NB_ITEMS ==  4);
    static_assert (myvector_t<int32_t,3>::CACHE_NB_ITEMS ==  8);
    static_assert (myvector_t<int32_t,4>::CACHE_NB_ITEMS == 16);
#endif

    auto operator() (int32_t n) const  {
        myvector_t<int32_t> data;
        for (int32_t i=1; i<=n; i++)  { data.push_back(i); }

        uint32_t sum1=0;  for (auto x : data)                   { sum1+=x; }
        uint32_t sum2=0;  for (size_t i=0; i<data.size(); i++)  { sum2+=data[i]; }

        data.push_back (sum1);
        data.push_back (sum2);

        return data;
    }
};
