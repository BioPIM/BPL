////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorSizeof12 : bpl::Task<ARCH>
{
    USING(ARCH);

    struct Key  { uint16_t a;  uint32_t b; };
    struct Seed { uint16_t a;  Key b;      };

    static_assert (sizeof(Seed)==12);

    using vector_t = vector<Seed>;

    auto operator() (vector_t const& v) const
    {
        tuple<uint64_t,uint64_t,uint64_t> result;

        for (auto x : v)
        {
            std::get<0>(result) += 1*x.a;
            std::get<1>(result) += 2*x.b.a;
            std::get<2>(result) += 3*x.b.b;
        }

        return result;
    }
};
