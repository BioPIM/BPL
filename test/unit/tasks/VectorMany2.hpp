////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorMany2 : bpl::Task<ARCH>
{
    struct config  {
        static const int VECTOR_MEMORY_SIZE_LOG2 = 10;
    };

    USING (ARCH,config);

    template<template<typename...> class CONTAINER, typename T>
    struct type_generic
    {
        CONTAINER<T> a0;
        CONTAINER<T> a1;
        CONTAINER<T> a2;
        CONTAINER<T> a3;
        CONTAINER<T> a4;
        CONTAINER<T> a5;
        CONTAINER<T> a6;
        CONTAINER<T> a7;
        CONTAINER<T> a8;
        CONTAINER<T> a9;
    };

    using type = type_generic<vector_view,uint32_t>;

    auto operator() (const type& t)
    {
        vector<uint32_t> c;

        for (auto x : t.a0)   { c.push_back(x); }
        for (auto x : t.a1)   { c.push_back(x); }
        for (auto x : t.a2)   { c.push_back(x); }
        for (auto x : t.a3)   { c.push_back(x); }
        for (auto x : t.a4)   { c.push_back(x); }
        for (auto x : t.a5)   { c.push_back(x); }
        for (auto x : t.a6)   { c.push_back(x); }
        for (auto x : t.a7)   { c.push_back(x); }
        for (auto x : t.a8)   { c.push_back(x); }
        for (auto x : t.a9)   { c.push_back(x); }

        return c;
    }
};
