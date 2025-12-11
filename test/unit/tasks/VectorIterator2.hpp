////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorIterator2: public bpl::Task<ARCH>
{
    struct Config  {  static constexpr bool SHARED_ITER_CACHE = false;  };

    USING(ARCH,Config);

#ifdef DPU
	static_assert (vector_view<uint64_t>::is_shared_iter_cache == false);
#endif

    auto operator() (vector_view<uint64_t> const& v) const
    {
        vector<tuple<uint64_t,uint64_t,uint64_t,uint64_t>> result;

        auto b1 = v.begin();   auto e1 = v.end();
        auto b2 = v.begin();   auto e2 = v.end();
        auto b3 = v.begin();   auto e3 = v.end();
        auto b4 = v.begin();   auto e4 = v.end();

        for (auto it1=b1; it1!=e1; ++it1)
        {
            for (auto it2=b2; it2!=e2; ++it2)
            {
                for (auto it3=b3; it3!=e3; ++it3)
                {
                    for (auto it4=b4; it4!=e4; ++it4)
                    {
                        result.push_back (make_tuple(*it1,*it2,*it3,*it4));
                    }
                }
            }
        }
        return result;
    }
};

