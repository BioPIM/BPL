////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_UTILS_SPLIT_HPP__
#define __BPL_UTILS_SPLIT_HPP__

#include <type_traits>
#include <vector>
#include <span>

////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct splitable {  static constexpr int value = true;  };

template<typename T>
static constexpr bool is_splitable_v = splitable <T>::value;

////////////////////////////////////////////////////////////////////////////////
template <typename T>
requires (is_splitable_v<T>)
struct SplitOperator
{
    /** By default, no split, ie just return the incoming parameter. */
    static auto split (const T& t, std::size_t idx, std::size_t total)
    {
        DEBUG_SPLIT ("SPLIT GENERIC  sizeof: %2d  idx: %5d  total: %5d \n",
            (uint32_t) sizeof(T), (uint32_t)idx, (uint32_t)total
        );

        return t;
    }

    static auto split2 (const T& t, std::size_t idx, std::size_t total)
    {  return split (t, idx, total);  }
};

////////////////////////////////////////////////////////////////////////////////
template <typename A,typename B>
struct SplitOperator<std::pair<A,B>>
{
    static auto split (const std::pair<A,B>& t, std::size_t idx, std::size_t total)
    {
        size_t diff = t.second - t.first;
        size_t i0   = diff * (idx+0) / total;
        size_t i1   = diff * (idx+1) / total;

//        DEBUG_SPLIT ("SPLIT<pair> sizeof: [%2d,%2d]  idx: %5d  total: %5d   in: [%5d %5d]  =>  out: [%5d %5d]\n",
//            sizeof(A), sizeof(B), idx, total, t.first, t.second, i0, i1);

        return std::pair { t.first+i0, t.first+i1 };
    }

    static auto split2 (const std::pair<A,B>& t, std::size_t idx, std::size_t total)
    {  return split (t, idx, total);  }
};

////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct SplitOperator<std::vector<T>>
{
    static auto split (const std::vector<T>& t, std::size_t idx, std::size_t total)
    {
        auto range    = std::make_pair (0, t.size());
        auto subrange = SplitOperator<decltype(range)>::split (range, idx, total);

        DEBUG_SPLIT ("SPLIT<vector>  sizeof: %2d  idx: %5d  total: %5d  [%5d %5d]  => [%5d %5d]\n",
            (uint32_t) sizeof(T),     (uint32_t)idx, (uint32_t)total,
            (uint32_t)range.first,    (uint32_t)range.second,
            (uint32_t)subrange.first, (uint32_t)subrange.second
        );

        return std::vector<T> (t.begin()+subrange.first, t.begin()+subrange.second);
    }

    static auto split2 (const std::vector<T>& t, std::size_t idx, std::size_t total)
    {
        return SplitOperator<std::span<T>>::split ((std::vector<T>&)t, idx, total);
    }
};

////////////////////////////////////////////////////////////////////////////////
template <typename T>
struct SplitOperator<std::span<T>>
{
    static auto split (std::span<T> t, std::size_t idx, std::size_t total)
    {
        auto range    = std::make_pair (size_t(0), t.size());
        auto subrange = SplitOperator<decltype(range)>::split (range, idx, total);

        auto res = t.subspan (subrange.first, subrange.second - subrange.first);

        DEBUG_SPLIT ("SPLIT<span>  sizeof: %2d  idx: %5d  total: %5d  [%5d %5d]  => [%5d %5d]   #res: %d\n",
            (uint32_t) sizeof(T), (uint32_t)idx, (uint32_t)total,
            (uint32_t)range.first,    (uint32_t)range.second,
            (uint32_t)subrange.first, (uint32_t)subrange.second,
            (uint32_t)res.size()
        );

        return res;
    }

    static auto split2 (std::span<T> t, std::size_t idx, std::size_t total)
    {  return split (t, idx, total);  }
};

////////////////////////////////////////////////////////////////////////////////
#endif /* __BPL_UTILS_SPLIT_HPP__ */
