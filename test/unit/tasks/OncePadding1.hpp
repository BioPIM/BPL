////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct OncePadding1 : public bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (
        once<vector<uint64_t> const&> v1,
             vector<uint64_t> const&  v2
    ) const
    {
        // since v1 is split from the host side (at DPU level), we may have different sizes for v1 for different DPUs
        return make_tuple (v1->size(), accumulate(*v1, uint64_t{0}), accumulate(v2, uint64_t{0}));
    };
};
