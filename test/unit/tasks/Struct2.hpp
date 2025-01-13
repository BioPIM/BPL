////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class ARCH>
struct Struct2
{
    USING(ARCH);

    struct MyStruct
    {
        vector<uint16_t> a1;
    };

    auto operator() (const MyStruct& x)
    {
        return accumulate (x.a1, uint64_t(0));
    }

    static auto split (const MyStruct& t, size_t idx, size_t total)
    {
        return MyStruct { SplitOperator<decltype(t.a1)>::split (t.a1, idx, total) };
    }

    static auto split_view (const MyStruct& t, size_t idx, size_t total)
    {
        return std::make_tuple (SplitOperator<decltype(t.a1)>::split_view (t.a1, idx, total));
    }

    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
