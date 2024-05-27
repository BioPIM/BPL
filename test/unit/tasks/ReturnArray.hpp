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
struct ReturnArray : bpl::core::Task<ARCH>
{
    USING(ARCH);

    using type_t    = uint32_t;
    using myarray_t = array<type_t,64>;

    auto operator() (const myarray_t& data, type_t K)
    {
        myarray_t result;

        for (size_t i=0; i<data.size(); i++)
        {
            result[i] = data[i] + K + this->tuid();
        }

        return result;
    }
};
