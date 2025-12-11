////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorIterator3: public bpl::Task<ARCH>
{
    struct Config  {  static constexpr bool SHARED_ITER_CACHE = false;  };

    USING(ARCH,Config);

    using type = uint32_t;

#ifdef DPU
    static_assert (vector_view<type>::is_shared_iter_cache == Config::SHARED_ITER_CACHE);
    static_assert (vector     <type>::is_shared_iter_cache == Config::SHARED_ITER_CACHE);
#endif

    auto operator() (vector_view<type> const& v)
    {
        vector<pair<type,type>> result;
        for (auto it1=v.begin(); it1!=v.end(); ++it1)  {
            for (auto it2=it1+1; it2!=v.end(); ++it2)  {
                result.push_back (make_pair(*it1,*it2));
            }
        }
        return result;
    }
};

