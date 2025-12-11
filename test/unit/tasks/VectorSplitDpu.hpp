////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorSplitDpu : public bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (
        vector<uint64_t> const& v1,
        vector<uint64_t> const& v2
    ) const
    {
        return make_pair (accumulate(v1, uint64_t{0}), accumulate(v2, uint64_t{0}));
    }
};
