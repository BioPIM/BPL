////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorSplitOverload : public bpl::Task<ARCH>
{
    USING(ARCH);

    uint64_t operator() (uint64_t value) const  {

        //printf ("tuid: %ld  value: %ld\n", uint64_t(this->tuid()), uint64_t(value));

        return value*value;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
