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
    static const int VECTOR_MEMORY_SIZE_LOG2 = 10;
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

if (this->tuid()==0)
{
#ifdef DPU
        using arg1_t = std::decay_t<decltype(a)>;
        using arg2_t = std::decay_t<decltype(b)>;

        printf ("1 => log2 sizeof: %3d  MEMORY_SIZE: %3d  CACHE_NB: %3d  CACHE_NB_ITEMS: %3d \n", bpl::Log2<sizeof(typename arg1_t::type)>::value, arg1_t::MEMORY_SIZE, arg1_t::CACHE_NB, arg1_t::CACHE_NB_ITEMS);
        printf ("2 => log2 sizeof: %3d  MEMORY_SIZE: %3d  CACHE_NB: %3d  CACHE_NB_ITEMS: %3d \n", bpl::Log2<sizeof(typename arg2_t::type)>::value,  arg2_t::MEMORY_SIZE, arg2_t::CACHE_NB, arg2_t::CACHE_NB_ITEMS);
#endif
}
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////
//constexpr bpl::Properties props
//{{{
//    {"vector.memory_size_log2", 6},
//    {"vector.cache_nb_log2",    0}
//}}};

