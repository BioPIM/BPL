////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <tasks/SortSelection.hpp>

#ifdef DPU
// Template specialization for UPMEM for our custom allocator.
template<typename Config,int NBITEMS_LOG2>
struct bpl::FixedAllocator<bpl::ArchUpmemResources<Config>,NBITEMS_LOG2>  {
    // Defining all the required constants for bpl::vector
    template<typename T>
    struct type {
        static const int  MEMORY_SIZE_LOG2               = NBITEMS_LOG2 + bpl::Log2<sizeof(T)>::value;
        static const int  CACHE_NB_LOG2                  = 1;
        static const bool SHARED_ITER_CACHE              = true;
        static const int  MEMTREE_MAX_MEMORY_LOG2        = 6;
        static const int  MEMTREE_NBITEMS_PER_BLOCK_LOG2 = 2;
    };
};
#endif

////////////////////////////////////////////////////////////////////////////////

template<class ARCH,typename T,int NBITEMS_LOG2>
using allocator_t = typename bpl::FixedAllocator<ARCH,NBITEMS_LOG2>::template type<T>;

template<class ARCH, typename T>
using vector_t = typename ARCH::template vector<T, allocator_t<ARCH,T,6>>;

template<class ARCH>
struct SortSelectionVector
     : SortSelection<ARCH /*, vector_t<ARCH,uint32_t> */>  {};
