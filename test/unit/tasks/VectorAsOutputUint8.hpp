////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorAsOutputUint8 : bpl::core::Task<ARCH>
{
    USING(ARCH);

    using type_t   = uint8_t;
    using vector_t = vector<type_t>;

    auto operator() (uint32_t nbitems)
    {
        vector_t result;

        // NOTE: we use the 'tuid' in order to have a specific result for each tasklet

        // WARNING!!! we must make sure that 'i' can't reach the max value for its type
        // -> if we use uint8_t, the last valid value in the loop must be 254.
        // -> if we allow to be equal to 255 in the loop, the i++ will make it go back to 0
        //    leading to an infinite loop

        for (type_t i=this->tuid(); i <= nbitems+2*this->tuid(); i++)  {  result.push_back (i); }

        return result;
    }
};
