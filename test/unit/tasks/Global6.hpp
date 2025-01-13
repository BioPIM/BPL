////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class ARCH>
struct Global6
{
    USING(ARCH);

    auto operator() (global<const vector<uint32_t>&> v)  // 'global' will transform 'vector' into 'vector_view'
    {
        uint64_t result = 0;

        for (auto x : *v)  {  result += x; }

        return result;
    }
};
