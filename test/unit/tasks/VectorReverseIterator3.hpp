////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<typename ARCH>
static auto VectorReverseIterator3_aux (
    int tuid,
    auto const& ref,
    auto const& qry,
    size_t shift
)
{
    size_t nberrors = 0;

    auto extend_fct = [&] (auto&& itRef, auto&& endRef, auto&& itQry, auto&& endQry)  {
        std::size_t nb=0;
        for (size_t pos=0; itRef!=endRef and itQry!=endQry; ++itRef, ++itQry, ++pos)  {
            if (*itRef != *itQry)  { nberrors++; }
            nb++;
        }
        return nb;
    };

    auto nb = extend_fct (
        ARCH::make_reverse_iterator(ref.begin() + shift),  ARCH::make_reverse_iterator(ref.begin()),
        ARCH::make_reverse_iterator(qry.begin() + shift),  ARCH::make_reverse_iterator(qry.begin())
    );

    return std::make_pair(nb==shift, nberrors);
}

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorReverseIterator3 : bpl::Task<ARCH>
{
    struct Config  {
        static constexpr bool SHARED_ITER_CACHE = false;
    };

    USING(ARCH,Config);

    using type = uint32_t;

    auto operator() (
        vector_view<type> const& ref,
        vector_view<type> const& qry,
        size_t shift
    ) const
    {
        return VectorReverseIterator3_aux<ARCH> (this->tuid(), ref,qry,shift);
    }
};
