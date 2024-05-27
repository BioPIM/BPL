////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/utils/Range.hpp>

using namespace bpl::core;

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Syracuse
{
    USING(ARCH);

    using range_t = Range;

    static auto run (int puid, const range_t& range)
    {
        uint64_t result = 0;

        for (uint32_t n : range)
        {
            size_t steps = 0;

            // we compute the number of steps that makes the recursion go to 1
            for ( ; n!=1; steps++)
            {
                if ( (n & 1) == 0)  {  n =  n >> 1;  }
                else                {  n = 3*n + 1;  }
            }

            result += steps;
        }

        return result;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
