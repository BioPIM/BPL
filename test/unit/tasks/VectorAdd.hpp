////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
// @description: Takes two vectors Ai and Bi as input and computes an output
// vector Ci=Ai+Bi
// @remarks: a 'zip' function is used for iterating Ai and Bi
// @benchmark-input: 2^n for n in range(10,20)
// @benchmark-split: yes (v1 and v2)
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorAdd
{
    USING(ARCH);

    auto operator() (const vector_view <uint32_t>& v1, const vector_view <uint32_t>& v2)
    {
        vector<uint32_t> result;

        for (auto [x,y] : zip(v1,v2))  { result.push_back (x+y); }

        return result;
    }
};
