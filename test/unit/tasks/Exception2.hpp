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
struct Exception2 : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (uint16_t tuid, uint64_t nbitems) const
    {
        vector<uint64_t> v;

        // We want to add nbitems to the vector for the current tasklet.
        // For instance nbitems=10*1024*1024 will go beyond the MRAM

        if (this->tuid() == tuid)
        {
            for (uint64_t i=0; i<nbitems; i++)
            {
                v.push_back (i);
            }
        }

        return 0;
    }
};
