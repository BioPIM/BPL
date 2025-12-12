////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
// @description: Takes a vector as input, iterates its content and compute the
// checksum. The final result is reduced checksum. We tag the vector with 'once'
// in order to test successive calls
// @remark: the first call is not used for time statistics in order to assess
// the impact of 'once'
// @benchmark-input: 2^n for n in 20,22,24,26,28
// @benchmark-split: yes
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorChecksumOnce : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (once<vector<uint32_t> const&> v)
    {
        uint64_t checksum = 0;
        for (auto x : *v)  {  checksum += x;  }
        return checksum;
    }

    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};
