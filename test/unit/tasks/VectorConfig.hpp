////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
struct SomeConfig
{
    static const int VECTOR_MEMORY_SIZE_LOG2 = 10;
    static const int VECTOR_CACHE_NB_LOG2    = 1;
};

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorConfig : public bpl::Task<ARCH>
{
    USING(ARCH,SomeConfig);

    auto operator() (const vector<uint64_t>& a, const vector<uint8_t>& b)
    {
        vector<uint32_t> c;

        return 0;
    }
};
