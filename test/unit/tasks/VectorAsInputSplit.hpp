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
struct VectorAsInputSplit
{
    USING(ARCH);

    using type_t   = uint32_t;
    using vector_t = vector<type_t>;

    auto operator() (const vector_t& input, uint32_t imax)
    {
        uint64_t sum = 0;

        for (uint32_t i=1; i<=imax; i++)
        {
            for (auto x : input)  {  sum += x;  }
        }

        return sum / imax;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
