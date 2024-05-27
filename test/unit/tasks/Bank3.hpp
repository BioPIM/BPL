////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>
#include <bpl/bank/BankChunk.hpp>

#include <tasks/Bank2.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Bank3 : Bank2<ARCH>
{
    USING(ARCH);

    using result_t = typename Bank2<ARCH>::result_t;

    static auto reduce (const result_t& a, const result_t& b)
    {
        return result_t {
            get<0>(a) + get<0>(b),
            get<1>(a) + get<1>(b),
            get<2>(a) + get<2>(b),
            get<3>(a) + get<3>(b)
        };
    }
};
