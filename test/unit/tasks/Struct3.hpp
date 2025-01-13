////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class ARCH>
struct Struct3
{
    USING (ARCH);

    struct type
    {
        vector<uint16_t> a1;
        vector<uint32_t> a2;
        vector<uint64_t> a3;
    };

    auto operator() (const type& x)
    {
        uint64_t res = 0;
        for (auto i : x.a1)  { res += i; }
        for (auto i : x.a2)  { res += i; }
        for (auto i : x.a3)  { res += i; }
        return res;
    }

    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};
