////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

using namespace bpl;

////////////////////////////////////////////////////////////////////////////////
// @description: This test checks the Syracuse conjecture by computing the total
// stopping time for integers from 1 to N. Each task takes a range of integers
// and computes the total stopping time T for each integer in the range. All
// results are reduced into a single integer and thus with low broadcast
// exchange.
// @remarks: We both study the impact of pure calculus (ie. computing total
// stopping time) with potentially low result broadcast size.
// @benchmark-input: 2^n for n in range(20,31)
// @benchmark-split: yes (range)
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct SyracuseReduce : bpl::Task<ARCH>
{
    USING(ARCH);

    using range_t = pair<uint64_t,uint64_t>;

    auto operator() (range_t range) const
    {
        auto fct = [] (auto n) {
            size_t nb = 0;
            for ( ; n!=1; nb++)  {
                if ((n%2) == 0)  {  n = n/2;   }
                else             {  n = 3*n+1; }
            }
            return nb;
        };

        uint64_t result = 0;
        for (uint32_t n=range.first; n<range.second; ++n)  {
            result += fct(n);
        }
        return result;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
