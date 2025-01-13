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
struct VectorViewRandomAccess2
{
    USING(ARCH);

    auto operator() (vector_view<uint32_t> const& v) const
    {
        uint64_t result = 0;

        // the size of v must be a multiple of two => force cache reading at each access...
        for (size_t i=0; i<v.size()/2; i++)
        {
            result += v[i];
            result += v[v.size()-1-i];
        }

        return result;
    }
};
