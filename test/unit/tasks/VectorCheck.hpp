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
struct VectorCheck : bpl::core::Task<ARCH>
{
    USING(ARCH);

    using type_t   = uint32_t;
    using vector_t = vector<type_t>;

    auto operator() (uint32_t nbitems)
    {
        vector_t v;

        uint64_t checksum = 0;

        // NOTE: we add the 'tuid' to the number of integers in order to have a specific result for each tasklet
        for (uint32_t i=1; i <= nbitems+this->tuid(); i++)  { v.push_back (i); }

        for (type_t x : v)   {  checksum += x;  }

        // NOTE: we specify the tuple types (otherwise issue during serialization)
        return make_tuple<uint32_t,uint64_t> (v.size(), checksum);
    }
};
