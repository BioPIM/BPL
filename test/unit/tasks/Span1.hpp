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
struct Span1 : bpl::core::Task<ARCH>
{
    USING(ARCH);

    using type = vector<uint32_t>;

    auto operator() (const span<type>& sp)
    {
        uint32_t res = 0;
        for (const auto& v : sp)
        {
            for (auto x : v)  {  res += x*v.size(); }
        }
        return res;
    }

    static auto reduce (uint32_t a, uint32_t b)  { return a+b; }
};
