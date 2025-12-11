////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
// @description: Each task creates a vector with a given number of items.
// A loop is used for assigning values to that vector that is eventually
// returned from the task.
// @remarks: Low input broadcast with high output broadcast
// @benchmark-input: 2^n for n in range(10,16)
// @benchmark-split: no
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorCreation : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (int nbitems) const
    {
        vector<uint32_t> result (nbitems);

        for (size_t i=0; i<result.size(); i++) {
            result[i] = this->tuid() + i;
        }

        return result;
    }
};
