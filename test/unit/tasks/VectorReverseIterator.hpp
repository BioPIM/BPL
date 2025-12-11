////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
// @description: Iterates an input vector backwards and fills an output vector
// with the iterated items.
// @benchmark-input: 2^n for n in range(10,18)
// @benchmark-split: no
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorReverseIterator : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (vector_view<uint32_t> const& v, size_t shift) const
    {
        vector<uint32_t> result;

        auto b = v.rbegin() + shift;
        auto e = v.rend  ();

        for (auto it=b; it!=e ; ++it)
        {
            result.push_back (*it);
        }

        return result;
    }
};
