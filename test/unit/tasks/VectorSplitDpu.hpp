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
        if (this->tuid()==16)
        {
            printf ("tuid: %3ld  #v1: %3ld  #v2: %3ld\n", uint64_t(this->tuid()),  uint64_t(v1.size()), uint64_t(v2.size()) );
            for (auto x : v2)  { printf ("==> %ld\n", uint64_t(x)); }
        }


        return make_pair (accumulate(v1, uint64_t{0}), accumulate(v2, uint64_t{0}));
    }
};
