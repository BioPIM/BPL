////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
// @description: The task takes as input a (sorted) vector and reverses its items
// by swapping them
// @benchmark-input: 2^n for n in range(10,18)
// @benchmark-split: yes
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorReverseInPlace : bpl::Task<ARCH>
{
    struct Config {
        static constexpr bool SWAP_USED = true;
        static constexpr bool VECTOR_SERIALIZE_OPTIM = false;
    };

    USING(ARCH,Config);

    using value_type = int32_t;

    // The incoming vector is supposed to be sorted in increasing order.
    // Its size has to be an even integer.
    // Note that we have to return 'decltype(auto)' ('auto' alone would decay).
    decltype(auto) operator() (vector<value_type>& v) const
    {
        for (size_t i=0; i<v.size()/2; i++)  {  swap (v[i], v[v.size()-1-i]); }
        return v;
    }
};
