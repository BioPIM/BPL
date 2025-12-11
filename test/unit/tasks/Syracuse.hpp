////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

using namespace bpl;

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Syracuse : bpl::Task<ARCH>
{
    USING(ARCH);

    using range_t = pair<uint64_t,uint64_t>;

    auto operator() (range_t range) const
    {
        vector<uint32_t> result (range.second-range.first);

        auto fct = [] (auto x) {
            size_t nb = 0;
            for (uint32_t u=x; u!=1; )  {
                if ( (u&1) == 0)  {  u = u >> 1;  }
                else              {  u = 3*u+1;   }
                nb++;
            }
            return nb;
        };

        for (uint32_t x=range.first; x<range.second; ++x)  { result[x-range.first]=fct(x); }

        return result;
    }
};
