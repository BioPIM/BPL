////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorAddAndIterate : public bpl::Task<ARCH>
{
    USING(ARCH);

    uint32_t operator() (uint32_t n) const
    {
        uint32_t ok = 0;

        vector<uint64_t> v;

        uint64_t truth = 0;
        for (uint64_t i=1; i<=n; i++)
        {
            v.push_back (i);
            truth  += i;

            uint64_t sum = 0;
            for (auto x : v)  {  sum += x; }

            if (sum == truth )  { ok++; }
        }

        return ok;
    }
};
