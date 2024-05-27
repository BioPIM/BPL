////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_UTILS_RANGE_HPP__
#define __BPL_UTILS_RANGE_HPP__

#include <bpl/utils/split.hpp>
#include <bpl/utils/lfsr.hpp>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/7185437/is-there-a-range-class-in-c11-for-use-with-range-based-for-loops

class Range
{
 public:

    using type = long int;

   class iterator
   {
      friend class Range;

    public:
      type operator *() const { return i_; }
      const iterator &operator ++() { ++i_; return *this; }
      iterator operator ++(int) { iterator copy(*this); ++i_; return copy; }

      bool operator ==(const iterator &other) const { return i_ == other.i_; }
      bool operator !=(const iterator &other) const { return i_ != other.i_; }

    protected:
      iterator(type start) : i_ (start) { }

    private:
      type i_;
   };

   iterator begin() const { return begin_; }
   iterator end  () const { return end_;   }

   Range(type begin=type(), type end=type()) : begin_(begin), end_(end) {}

private:
   iterator begin_;
   iterator end_;
};

////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
class RakeRange
{
 public:

    using type = long int;

   class iterator
   {
      friend class RakeRange;

    public:
      type operator *() const { return i_; }
      const iterator &operator ++() { i_ += modulo_; return *this; }
      iterator operator ++(int) { iterator copy(*this); i_ += modulo_; return copy; }

      bool operator !=(const iterator &other) const { return i_ < other.i_; }
      bool operator ==(const iterator &other) const { return not this->operator !=(other); }

      type modulo() { return modulo_; }

    protected:
      iterator (type start, type modulo) : i_ (start), modulo_(modulo) { }

    private:
      type i_;
      type modulo_;
   };

   iterator begin() const { return begin_; }
   iterator end()   const { return end_;   }

   RakeRange (type begin=type(), type end=type(), type modulo=1) : begin_(begin, modulo), end_(end, modulo)
   {
       //assert (modulo > 0);
   }

private:
   iterator begin_;
   iterator end_;
};


//////////////////////////////////////////////////////////////////////////////////////////
// only for range being a power of 2
//////////////////////////////////////////////////////////////////////////////////////////
class RandomRange
{
 public:

    using type = uint32_t;

   class iterator
   {
      friend class RandomRange;

    public:
      type operator *() const { return i_; }
      const iterator &operator ++() { i_ = uint64_t(a_*i_+b_) % n_; return *this; }

      bool operator !=(const iterator &other) const { return i_ != other.i_; }
      bool operator ==(const iterator &other) const { return not this->operator !=(other); }

    protected:
      iterator (type start, type a, type b, type n) : i_ (start), a_(a), b_(b), n_(n) { }

    private:
      type i_;
      type a_;
      type b_;
      type n_;
   };

   iterator begin() const { return begin_; }
   iterator end()   const { return end_;   }

   RandomRange (type begin=type(), type end=type(), type a=1, type b=0) : begin_(begin, a,b,end-begin), end_(end, a,b,end-begin)
   {
   }

private:
   iterator begin_;
   iterator end_;
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
template <>
struct SplitOperator<bpl::core::Range>
{
    static auto split (const bpl::core::Range& r, std::size_t idx, std::size_t total)
    {
        if (total==0)  { return r; }
        auto diff = *r.end() - *r.begin();
//printf ("==> split Range:  idx=%d  total=%ld  diff=%ld in=[%3ld:%3ld] -> out=[%3ld:%3ld]   check=%ld\n",
//    uint64_t(idx), uint64_t(total), uint64_t(diff), uint64_t(*r.begin()), uint64_t(*r.end()),
//    uint64_t(*r.begin() + diff * idx / total),
//    uint64_t(*r.begin() + diff * (idx + 1) / total),
//    uint64_t(123456789)
//);

        return bpl::core::Range (*r.begin() + diff * idx / total, *r.begin() + diff * (idx + 1) / total);
    }
};

//////////////////////////////////////////////////////////////////////////////////////////
template <>
struct SplitOperator<bpl::core::RakeRange>
{
    static auto split (const bpl::core::RakeRange& r, std::size_t idx, std::size_t total)
    {
        if (total==0)  { return r; }
        return bpl::core::RakeRange (*r.begin()+idx, *r.end(), total);
    }
};

//inline auto split<bpl::core::Range> (const bpl::core::Range& r, std::size_t idx, std::size_t total)
//{
//    if (total==0)  { return r; }
//    auto diff = *r.end() - *r.begin();
//    return bpl::core::Range (*r.begin() + diff * idx / total, *r.begin() + diff * (idx + 1) / total);
//}

//////////////////////////////////////////////////////////////////////////////////////////
//template<>
//inline auto split<bpl::core::RakeRange> (const bpl::core::RakeRange& r, std::size_t idx, std::size_t total)
//{
//    if (total==0)  { return r; }
//    return bpl::core::RakeRange (*r.begin()+idx, *r.end(), total);
//}

#endif /* __BPL_UTILS_RANGE_HPP__ */
