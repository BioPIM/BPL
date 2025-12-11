////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorMany4 : bpl::Task<ARCH>
{
    USING (ARCH);

    template<template<typename...> class CONTAINER, typename T>
    struct type_generic
    {
        CONTAINER<T> a1;
        CONTAINER<T> a2;
        CONTAINER<T> a3;
    };

    template<typename T>  using type = type_generic<vector_view,T>;

    using type8  = type<uint8_t >;
    using type16 = type<uint16_t>;
    using type32 = type<uint32_t>;

    auto operator() (type8 const& t, type16 const& u, type32 const& v)
    {
        vector<uint32_t> c;

        for (auto x : t.a1)   { c.push_back(x); }
        for (auto x : t.a2)   { c.push_back(x); }
        for (auto x : t.a3)   { c.push_back(x); }

        for (auto x : u.a1)   { c.push_back(x); }
        for (auto x : u.a2)   { c.push_back(x); }
        for (auto x : u.a3)   { c.push_back(x); }

        for (auto x : v.a1)   { c.push_back(x); }
        for (auto x : v.a2)   { c.push_back(x); }
        for (auto x : v.a3)   { c.push_back(x); }

        return c;
    }
};
