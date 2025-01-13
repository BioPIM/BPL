////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>
#include <bpl/utils/RangeInt.hpp>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct SplitRangeInt : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (bpl::RangeInt range)
    {
        lock_guard<mutex> lock (this->get_mutex(0));

        auto result = accumulate (range, uint64_t(0));

        this->log ("tuid: %3d  nbcycles: %4d\n", this->tuid(), this->nbcycles());

        return result;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
