////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorAsOutputUint32 : bpl::Task<ARCH>
{
    USING(ARCH);

    using type_t   = uint32_t;
    using vector_t = vector<type_t>;

    auto operator() (uint32_t nbitems)
    {
        vector_t result;
        for (type_t i=this->tuid(); i <= nbitems+2*this->tuid(); i++)  {  result.push_back (i); }
        return result;
    }
};
