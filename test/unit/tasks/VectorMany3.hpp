////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorMany3 : bpl::Task<ARCH>
{
    USING (ARCH);

    struct type
    {
        vector_view<uint32_t> a0;
        vector_view<uint32_t> a1;
        vector_view<uint32_t> a2;
        vector_view<uint32_t> a3;
        vector_view<uint32_t> a4;
        vector_view<uint32_t> a5;
        vector_view<uint32_t> a6;
        vector_view<uint32_t> a7;
        vector_view<uint32_t> a8;
        vector_view<uint32_t> a9;
    };

    auto operator() (global<const type&> t)
    {
        vector<uint32_t> c;

        for (auto x : t->a0)   { c.push_back(x); }
        for (auto x : t->a1)   { c.push_back(x); }
        for (auto x : t->a2)   { c.push_back(x); }
        for (auto x : t->a3)   { c.push_back(x); }
        for (auto x : t->a4)   { c.push_back(x); }
        for (auto x : t->a5)   { c.push_back(x); }
        for (auto x : t->a6)   { c.push_back(x); }
        for (auto x : t->a7)   { c.push_back(x); }
        for (auto x : t->a8)   { c.push_back(x); }
        for (auto x : t->a9)   { c.push_back(x); }

        return c;
    }
};
