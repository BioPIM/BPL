////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorAsInput3
{
    USING(ARCH);

    using type_t   = uint32_t;
    using vector_t = vector<type_t>;

    auto operator() (vector_t& v1, vector_t& v2)
    {
        uint64_t sum = 0;

        for (auto x : v1)
        {
            for (auto y : v2)  {  sum += x + y;  }
        }

        return sum;
    }

    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};
