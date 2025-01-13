////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/utils/metaprog.hpp>
#include <bpl/utils/splitter.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

class RangeInt
{
public:

    static constexpr bool parseable = false;

    using size_type = uint32_t;

private:

    struct iterator
    {
        iterator (size_type idx) : idx_(idx)  {}
        auto operator*() const { return idx_; }
        iterator& operator++ () { ++idx_; return *this; }
        bool operator!=(const iterator& other) const { return idx_!=other.idx_; }
        size_type idx_;
    };

public:

    RangeInt () = default;

    RangeInt (size_type first, size_type last) : bounds_(first,last)  {}

    auto begin () const { return iterator (bounds_.first); }
    auto end   () const { return iterator (bounds_.second);  }

    auto size() const { return bounds_.second - bounds_.first; }

    auto first() const { return bounds_.first;  }
    auto last () const { return bounds_.second; }

    std::pair<size_type,size_type> bounds_;
};

//////////////////////////////////////////////////////////////////////////////
template<>
struct serializable<RangeInt> : std::true_type
{
    template<class ARCH, class BUFITER, int ROUNDUP, typename T, typename FCT>
    static auto iterate (bool transient, int depth, const T& t, FCT fct, void* context=nullptr)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (false, depth+1, t.bounds_, fct, context);
    }

    template<class ARCH, class BUFITER, int ROUNDUP, typename T>
    static auto restore (BUFITER& it, T& result)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, result.bounds_);
    }
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
template<>
struct SplitOperator<bpl::RangeInt>
{
    static auto split (const bpl::RangeInt& x, std::size_t idx, std::size_t total)
    {
        size_t i0 = x.size() * (idx+0) / total;
        size_t i1 = x.size() * (idx+1) / total;

        return bpl::RangeInt (x.first() + i0, x.first() + i1);
    }

    static auto split_view (const bpl::RangeInt& x, std::size_t idx, std::size_t total)
    {  return split (x, idx, total);  }
};


