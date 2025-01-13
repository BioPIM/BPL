////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorAsInputSplit : bpl::Task<ARCH>
{
    USING(ARCH);

    using type_t   = uint32_t;
    using vector_t = vector<type_t>;

    auto operator() (vector_t const& input, uint32_t imax) const
    {
        uint64_t sum = 0;

        for (uint32_t i=1; i<=imax; i++)
        {
            for (auto x : input)  {  sum += x;  }
        }
        
        return sum / imax;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
