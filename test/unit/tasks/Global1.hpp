////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

template<class ARCH>
struct Global1 : bpl::core::Task<ARCH>
{
    USING(ARCH);

    using type = array<uint16_t,64>;

    auto operator() (global<type> values)
    {
        return bpl::core::accumulate (*values, uint64_t(0));
    }

    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};
