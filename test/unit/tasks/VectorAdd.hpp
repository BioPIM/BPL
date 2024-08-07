////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

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
