////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorMany1 : public bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (
        global<const vector_view<uint32_t>&> a0,
        global<const vector_view<uint32_t>&> a1,
        global<const vector_view<uint32_t>&> a2,
        global<const vector_view<uint32_t>&> a3,
        global<const vector_view<uint32_t>&> a4,
        global<const vector_view<uint32_t>&> a5,
        global<const vector_view<uint32_t>&> a6
    )
    {
        vector<uint32_t> c;

        for (auto x : *a0)  { c.push_back(x); }
        for (auto x : *a1)  { c.push_back(x); }
        for (auto x : *a2)  { c.push_back(x); }
        for (auto x : *a3)  { c.push_back(x); }
        for (auto x : *a4)  { c.push_back(x); }
        for (auto x : *a5)  { c.push_back(x); }
        for (auto x : *a6)  { c.push_back(x); }

        return c;
    }
};
