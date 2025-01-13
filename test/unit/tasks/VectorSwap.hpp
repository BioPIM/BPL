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
struct VectorSwap : bpl::Task<ARCH>
{
    struct Config
    {
        // We need explicitly to define at least two vector caches for 'swap' usage
        // otherwise a compilation error involving type ErrorOnlyOneCacheForSwapUsage will occur.
        //
        // From performance issue, it is better IN GENERAL to use only one cache, and so one makes it
        // mandatory for the user to declare the intent to use swap through SWAP_USED constant definition.
        static constexpr bool SWAP_USED = true;
    };

    USING(ARCH,Config);

    using result_t = vector<uint32_t>;

    result_t operator() (uint32_t n) const
    {
        result_t result;

        // We fill the vector with integer 1..2n
        for (uint32_t i=1; i<=2*n; i++)  {  result.push_back (i);  }

        // We reverse the vector by using 'swap'
        for (uint32_t i=0; i<n; i++)
        {
            swap (result[i], result[result.size()-1-i]);
        }

        // We return the result.
        return result;
    }
};
