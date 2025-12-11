////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

template<class ARCH>
struct ResultsToVector : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (size_t n)
    {
        vector<uint16_t> result;

        for (size_t i=1; i<=n+this->tuid(); i++)
        {
            result.push_back (i);
        }

        return result;
    }
};
