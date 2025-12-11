////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct IterateMultipleVectorsStruct
{
    USING(ARCH);

    vector_view<uint32_t> d1;
    vector_view<uint32_t> d2;
};

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct IterateMultipleVectors : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (IterateMultipleVectorsStruct<ARCH> const& object) const
    {
        return make_tuple (accumulate (object.d1, uint64_t{0}), accumulate (object.d2, uint64_t{0}));
    }
};
