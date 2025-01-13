////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class ARCH>
struct Once1
{
    USING(ARCH);

    auto operator() (once<vector<uint32_t> const&> v, uint32_t a)
    {
        static_assert (bpl::hastag_once_v<decltype(v)> == true);

        return bpl::accumulate (*v, uint64_t(0));
    }
};
