////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct DummyStruct
{
    USING(ARCH);

    vector_view<uint32_t> data;
};

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct SplitDifferentSizes : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (DummyStruct<ARCH> const& object) const
    {
        return make_tuple (
            this->guid(),
            accumulate (object.data, uint64_t{0})
        );
    }
};
