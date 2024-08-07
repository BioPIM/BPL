////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

template<class ARCH>
struct Global2 : bpl::core::Task<ARCH>
{
    USING(ARCH);

    using type1    = array<uint16_t,400>;
    using type2    = array<uint16_t,100>;
    using result_t = tuple<uint64_t,uint64_t>;

    result_t operator() (global<type1> v1, global<type2> v2)
    {
        return make_tuple (
            bpl::core::accumulate (*v1, uint64_t(0)),
            bpl::core::accumulate (*v2, uint64_t(0))
        );
    }

    static auto reduce (result_t a, result_t b)
    {
        return make_tuple (
            get<0>(a) + get<0>(b),
            get<1>(a) + get<1>(b)
        );
    }
};
