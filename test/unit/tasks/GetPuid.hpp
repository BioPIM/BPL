////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

template<class ARCH>
struct GetPuid : bpl::core::Task<ARCH>
{
    USING(ARCH);

    size_t operator() ()    { return this->tuid();  }

    static size_t reduce (int a, int b)    { return a+b;  }
};
