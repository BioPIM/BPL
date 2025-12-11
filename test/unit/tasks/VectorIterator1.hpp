////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorIterator1: public bpl::Task<ARCH>
{
    struct Config  {  static constexpr bool SHARED_ITER_CACHE = false;  };

    USING(ARCH,Config);

#ifdef DPU
	static_assert (vector_view<uint64_t>::is_shared_iter_cache == false);
#endif

    auto operator() (vector_view<uint64_t> const& v) const
    {
        vector<pair<uint64_t,uint64_t>> result;

        auto b1 = v.begin();   auto e1 = v.end();
        auto b2 = v.begin();   auto e2 = v.end();

        for (auto it1=b1; it1!=e1; ++it1)
        {
            for (auto it2=b2; it2!=e2; ++it2)
            {
                result.push_back (make_pair(*it1,*it2));
            }
        }
        return result;
    }
};

