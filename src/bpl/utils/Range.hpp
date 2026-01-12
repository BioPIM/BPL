////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/utils/split.hpp>
#include <bpl/utils/lfsr.hpp>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/7185437/is-there-a-range-class-in-c11-for-use-with-range-based-for-loops

/** \brief class that allows to iterate an range of integers.
 */
class Range
{
public:

    static constexpr bool parseable = false;

    using type = long int;

    using size_type = uint32_t;

private:

    struct iterator  {
        iterator (size_type idx) : idx_(idx)  {}
        auto operator*() const { return idx_; }
        iterator& operator++ () { ++idx_; return *this; }
        bool operator!=(const iterator& other) const { return idx_!=other.idx_; }
        size_type idx_;
    };

public:

    Range () = default;

    Range (size_type first, size_type last) : bounds_(first,last)  {}

    auto begin () const { return iterator (bounds_.first); }
    auto end   () const { return iterator (bounds_.second);  }

    auto size() const { return bounds_.second - bounds_.first; }

    auto first() const { return bounds_.first;  }
    auto last () const { return bounds_.second; }

    std::pair<size_type,size_type> bounds_;
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
/** \brief Template specialization of SplitOperator for the Range type.
 * \see bpl::SplitOperator. */
template<>
struct SplitOperator<bpl::Range>
{
    static auto split (const bpl::Range& x, std::size_t idx, std::size_t total)
    {
        size_t i0 = x.size() * (idx+0) / total;
        size_t i1 = x.size() * (idx+1) / total;

        return bpl::Range (x.first() + i0, x.first() + i1);
    }

    static auto split_view (const bpl::Range& x, std::size_t idx, std::size_t total)
    {  return split (x, idx, total);  }
};
