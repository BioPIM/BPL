////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
// @description: Selection sort on a given array.
// @benchmark-input: 2^n for n in range(20,24)
// @benchmark-split: yes (v)
// @benchmark-multicore-split: 1024
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct SortSelection : bpl::Task<ARCH>
{
    struct Config  {
        static constexpr bool SWAP_USED          = true;
        static constexpr bool SHARED_ITER_CACHE  = false;
        static const bool VECTOR_SERIALIZE_OPTIM = false;
    };

    USING(ARCH,Config);

    using vector_t = vector<uint32_t>;
    using value_type = typename vector_t::value_type;

    decltype(auto) operator() (vector_t& v) const
    {
        for (size_t i=0; i<v.size(); i++)  {
            size_t k=i;
            value_type x = v[i];
            size_t j=i+1;
            for (auto it2=v.begin()+j; it2!=v.end(); ++it2, ++j)  { if (*it2<x) {  k=j;  x=*it2;  } }
            swap (v[i], v[k]);
        }
        return v;
    }
};

