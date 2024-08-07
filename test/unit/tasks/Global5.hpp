////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class ARCH>
struct Global5
{
    USING(ARCH);

    auto operator() (global<const vector_view<uint32_t>&> v)
    {
        uint64_t result = 0;

        for (auto x : *v)  {  result += x; }

        return result;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
