////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
// @description: Takes a vector as input, iterates its content and compute the
// checksum. The final result is reduced checksum.
// @benchmark-input: 2^n for n in range(10,20)
// @benchmark-split: yes
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorChecksum : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (const vector<uint32_t>& v)
    {
        uint64_t checksum = 0;
        for (auto x : v)  {  checksum += x;  }
        return checksum;
    }

    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};
