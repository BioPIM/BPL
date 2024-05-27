////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

using namespace bpl::core;

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Benchmark2
{
    USING(ARCH);

    static auto run (int puid, pair<uint32_t,uint32_t> range)
    {
        uint64_t result = 0;

        for (uint32_t x=range.first; x<range.second; ++x)
        {
            size_t nb = 0;
            for (uint32_t u=x; u != 1; )
            {
                if ( (u&1) == 0)  {  u = u >> 1;  }
                else              {  u = 3*u+1;   }
                nb++;
            }

            result += nb;
        }

        return result;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
