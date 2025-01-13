////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct GlobalOnce3 : bpl::Task<ARCH>
{
    USING(ARCH);

    using type = vector_view<uint16_t> ;

    auto operator() (glonce<type const&> v, uint32_t k) const
    {
        return k * accumulate (*v, uint64_t(0));
    }
};
