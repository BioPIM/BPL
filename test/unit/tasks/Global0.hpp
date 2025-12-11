////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

template<class ARCH>
struct Global0 : bpl::Task<ARCH>
{
    USING(ARCH);

    using type = vector_view<uint32_t>;

    auto operator() (global<type const&> v)  {
        return  bpl::accumulate (*v, uint64_t{});
    }
};
