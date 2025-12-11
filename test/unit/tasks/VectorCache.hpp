////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
struct ArchConfig
{
    static const int VECTOR_MEMORY_SIZE_LOG2 = 9;
    static const int VECTOR_CACHE_NB_LOG2    = 1;
};

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorCache : public bpl::Task<ARCH>
{
    USING(ARCH,ArchConfig);

    auto operator() (const vector<uint64_t>& a, const vector<uint8_t>& b)
    {
        vector<uint32_t> c;

        return 0;
    }
};

//////////////////////////////////////////////////////////////////////
//constexpr bpl::Properties props
//{{{
//    {"vector.memory_size_log2", 6},
//    {"vector.cache_nb_log2",    0}
//}}};

