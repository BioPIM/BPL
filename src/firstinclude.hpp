////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <traces.hpp>

// We define a macro (empty by default) that can be used where required in order
// to enforce some memory alignment. For instance, UPMEM arch requires that WRAM buffers
// have to be __dma_aligned when used in direct MRAM access.
// A natural way to do this would be to define some type trait with different implementations
// according to the architecture.
// Unfortunately, "template<typename T> using arch_aligned_t =  __dma_aligned T;"
// doesn't seem to work, so we have to use a macro here.
#ifndef ARCH_ALIGN
  #define ARCH_ALIGN
#endif

#define NOINLINE __attribute__ ((noinline))
