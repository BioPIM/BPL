////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/core/Error.hpp>
#include <climits>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

struct exception
{
    std::size_t tuid = 0;
};

struct out_of_range  : exception {};
struct out_of_memory : exception {};

////////////////////////////////////////////////////////////////////////////////
// binding dynamic -> static
inline auto process_error (error_mask_t mask, size_t tuid)
{
    // We iterate each bit of the mask
    for (size_t b=0; mask>0; mask>>=1, b++)
    {
        //printf ("==> mask: 0x%lx  b: %ld   tuid: %ld \n", uint64_t(mask), uint64_t(b), uint64_t(tuid));

        if (mask & 1)
        {
            switch (static_cast<Error_e>(b))
            {
                case OUT_OF_RANGE :  throw out_of_range  { tuid };   break;
                case OUT_OF_MEMORY:  throw out_of_memory { tuid };   break;
                default: break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
