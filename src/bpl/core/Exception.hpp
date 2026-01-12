////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/core/Error.hpp>
#include <climits>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** \brief Pseudo-exceptions for DPU binaries
 *
 * The purpose for this structure was initially to make it possible to have
 * something like exceptions from the DPU binaries. It is not possible to
 * strictly have exceptions from DPU since the source code has to be compiled
 * with a c++ compiler with option -fno-exceptions. However, it seemed possible
 * to detect some errors during tasklets execution with an associated error code
 * to be broadcast to the host, then from the host convert this error code to
 * a specific exception. This mechanism never worked properly since it required
 * to kill a tasklet as soon as the error was detected, but an unresolved problem
 * remained with the other tasklets synchronization, so the whole idea was not
 * fully implemented.
 * \see bpl::Error_e
 */
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
