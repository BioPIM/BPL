////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>
#include <tasks/VectorReverseIterator3.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorReverseIterator3once : bpl::Task<ARCH>
{
    struct Config  {  static constexpr bool SHARED_ITER_CACHE = false; };

    USING(ARCH,Config);

    using type = typename VectorReverseIterator3<ARCH>::type;

    auto operator() (
        once<vector_view<type> const&> ref,
        vector_view<type> const& qry,
        size_t shift
    ) const
    {
        return VectorReverseIterator3_aux<ARCH> (this->tuid(),*ref,qry,shift);
    }
};
