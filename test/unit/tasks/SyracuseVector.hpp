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
// and computes the total stopping time T for each integer in the range. Each
// result is put into a vector that is returned as a result from the task.
// @remarks: We both study the impact of pure calculus (ie. computing total
// stopping time) and building a potentially big vector as a result of the
// task.
// @benchmark-input: 2^n for n in range(20,29)
// @benchmark-split: yes (range)
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct SyracuseVector : bpl::Task<ARCH>
{
    struct config  {  static const bool VECTOR_SERIALIZE_OPTIM = true;  };

    USING(ARCH,config);

    using range_t = pair<uint64_t,uint64_t>;

    auto operator() (range_t range) const
    {
        vector<uint32_t> result (range.second-range.first);

        auto fct = [] (auto n) {
            size_t nb = 0;
            for ( ; n!=1; nb++)  {
                if ( (n%2) == 0) {  n = n/2;   }
                else             {  n = 3*n+1; }
            }
            return nb;
        };

        for (uint32_t n=range.first; n<range.second; ++n)  {
            result[n-range.first] = fct(n);
        }

        return result;
    }
};
