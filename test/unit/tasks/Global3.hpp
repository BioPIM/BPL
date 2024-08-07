////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

template<class ARCH>
struct Global3 : bpl::core::Task<ARCH>
{
    USING(ARCH);

    using type1    = array<uint32_t,2000>;
    using type2    = array<uint16_t,256>;
    using type3    = array<uint64_t,4000>;

    using result_t = tuple<uint64_t,uint64_t,uint64_t>;

    result_t operator() (
        global<const type1&> v1,  // allocated in the WRAM (common to all tasklets)
               const type2&  v2,  // allocated in the stack of the current tasklet
        global<const type3&> v3   // allocated in the WRAM (common to all tasklets)
    )
    {
        return make_tuple (
            bpl::core::accumulate (*v1, uint64_t(0)),
            bpl::core::accumulate ( v2, uint64_t(0)),
            bpl::core::accumulate (*v3, uint64_t(0))
        );
    }

    static auto reduce (result_t a, result_t b)
    {
        return make_tuple (
            get<0>(a) + get<0>(b),
            get<1>(a) + get<1>(b),
            get<2>(a) + get<2>(b)
        );
    }
};
