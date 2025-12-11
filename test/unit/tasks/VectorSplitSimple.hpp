////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorSplitSimple : public bpl::Task<ARCH>
{
    USING(ARCH);

    //using value_type = uint64_t;
    using value_type = uint32_t;

    uint64_t operator() (vector_view<value_type> const& v) const  {
        return accumulate(v, uint64_t{0});
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
