////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>
#include <bpl/utils/Shared.hpp>
#include <bpl/utils/Reducer.hpp>

using namespace bpl::core;

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Mutex2 : Task<ARCH>, Shared<uint32_t>, Reducer<std::plus<uint64_t>>
{
    USING(ARCH);

    auto operator() (uint32_t n)
    {
        uint64_t sum = 0;

        for (uint32_t i=fetch_and_add(); i<=n;  i=fetch_and_add())
        {
            sum += i;
        }

        return sum;
    }

    uint32_t fetch_and_add ()
    {
        lock_guard<mutex> lock (this->get_mutex());
        return shared() ++;
    }
};
