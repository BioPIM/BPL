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
struct VectorInputOutput2  : public bpl::Task<ARCH>
{
    struct ArchConfig
    {
//        static const int VECTOR_MEMORY_SIZE_LOG2 = 8;
//        static const int VECTOR_CACHE_NB_LOG2    = 0;
    };

    USING(ARCH,ArchConfig);

    using vector_t = vector<uint32_t>;

    // IMPORTANT !!!
    // When using ArchUpmem, it seems that the compiler behaves differently according
    // to the presence of the return type in the prototype:
    //   (1) if 'auto' is used only, a (move) copy of the result is done
    //   (2) if explicit type 'vector_t' is used, no copy is done.
    // There is no explanation for this behavior for the moment.
    // Note that there is no such behavior when using ArchMulticore.
    auto operator() (const vector_t& v)  //-> vector_t
    {
        vector_t result;

//        if (this->tuid()==0)
//        {
//#ifdef DPU
//        using arg1_t = std::decay_t<decltype(v)>;
//        printf ("1 => sizeof: %d  log2 sizeof: %3d  MEMORY_SIZE: %3d  CACHE_NB: %3d  CACHE_NB_ITEMS: %3d \n",
//            sizeof(arg1_t), bpl::Log2<sizeof(typename arg1_t::type)>::value, arg1_t::MEMORY_SIZE, arg1_t::CACHE_NB, arg1_t::CACHE_NB_ITEMS);
//
//printf ("size: %d %d\n", v.size(), result.size());
//#endif
//        }

//        if (this->tuid()==0) {  for (auto i : v) {  printf ("val: %d\n", i); }  }

//        if (this->tuid()==0) {  for (uint32_t i=0; i<v.size(); i++)   {  printf ("val: %d\n", v[i]); }  }

        for (uint32_t i=0; i<v.size(); i++)
        {
            result.push_back (2*v[i]);
        }
#ifdef DPU
//        if (me()==0)  { printf ("=== operator()  %p\n", &result); }
//      result.flush();
#endif

        return result;
    }

    using result_type = bpl::return_t<decltype(&VectorInputOutput2<ARCH>::operator())>;
    static_assert (std::is_same_v<result_type,vector_t>);
};
