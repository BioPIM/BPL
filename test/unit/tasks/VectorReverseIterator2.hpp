////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <algorithm>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorReverseIterator2
{
    USING(ARCH);

    auto operator() (vector_view<uint32_t> const& v) const
    {
        vector<uint32_t> result;

        std::copy (
            make_reverse_iterator (v.end  ()),
            make_reverse_iterator (v.begin()),
            std::back_inserter (result)
        );

        return result;
    }
};
