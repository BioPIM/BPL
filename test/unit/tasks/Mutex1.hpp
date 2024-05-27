////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Mutex1 : bpl::core::Task<ARCH>
{
    USING(ARCH);

    auto operator() (uint32_t n)
    {
        uint64_t sum = 0;

        while (true)
        {
            lock_guard<mutex> lock (this->get_mutex());

            if (shared()<=n)  {  sum += shared() ++;  }
            else              {  break;               }
        }

        return sum;
    }

    static uint32_t& shared () { static uint32_t value = 1; return value; }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
