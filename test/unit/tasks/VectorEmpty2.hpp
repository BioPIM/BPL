////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////

template<class ARCH>
struct DataVectorEmpty
{
    USING(ARCH);

    using type = uint32_t;

    vector<type> a;
    vector<type> b;
    vector<type> c;
};


template<class ARCH>
struct VectorEmpty2 : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (DataVectorEmpty<ARCH> const& data)
    {
if (this->tuid()==0)
{
    printf ("sizes: %ld %ld %ld \n", uint64_t(data.a.size()), uint64_t(data.b.size()), uint64_t(data.c.size()));

    for (auto x : data.a)  { printf ("a: %ld\n", uint64_t(x)); }
    for (auto x : data.b)  { printf ("b: %ld\n", uint64_t(x)); }
    for (auto x : data.c)  { printf ("c: %ld\n", uint64_t(x)); }
}

        return make_tuple (
            accumulate (data.a, uint64_t(0)),
            accumulate (data.b, uint64_t(0)),
            accumulate (data.c, uint64_t(0))
        );
    }
};
