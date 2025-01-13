////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class ARCH>
struct GlobalOnce1
{
    USING(ARCH);

    using type = array<uint32_t,8000>;

    auto operator() (global<once<type const&>> v, uint32_t val) const
    {
        static_assert (bpl::hastag_global_v<decltype(v)> == true);
        static_assert (bpl::hastag_once_v  <decltype(v)> == true);

        return bpl::accumulate (*v, val);
    }
};
