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
struct VectorRandomAccessGlobal1 : bpl::Task<ARCH>
{
    USING(ARCH);

    using type = vector_view<uint16_t>;

    // => 'global' means that we have a common instance shared by the tasklets,
    // so using [] would concurrently use the same cache with potential wrong result

    auto operator() (glonce<type const&> data)  // glonce := global + once
    {
        uint64_t sum = 0;

        /// We use operator[] on purpose here.
        for (size_t i=0; i<data->size(); i++)  {  sum += (*data)[i];  }

        return sum;
    }
};
