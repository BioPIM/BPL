////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH, bool Value>
struct VectorSharedIterator
{
    struct Config  {  static constexpr bool SHARED_ITER_CACHE = Value;  };

    USING(ARCH,Config);

    using type = uint32_t;

#ifdef DPU
    static_assert (vector_view<type>::is_shared_iter_cache == Config::SHARED_ITER_CACHE);
    static_assert (vector     <type>::is_shared_iter_cache == Config::SHARED_ITER_CACHE);
#endif

    auto operator() (size_t nbitems) const
    {
        vector<type> result;
        for (type i=1; i<=nbitems; i++)  { result.push_back(i); }
        return result;
    }
};
