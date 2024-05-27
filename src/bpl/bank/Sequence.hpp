////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef _BPL_SEQUENCE_HPP_
#define _BPL_SEQUENCE_HPP_

#include <bpl/utils/serialize.hpp>
#include <bpl/utils/split.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace bank {
////////////////////////////////////////////////////////////////////////////////

template<std::size_t S>
struct Sequence
{
    using data_t = uint8_t;

    static const std::size_t SIZE = S;

    data_t data [SIZE];

    static constexpr data_t nucleotids[] = {'A', 'C', 'T', 'G' };

    size_t size() const { return SIZE; }

    template<typename FUNCTOR>
    void iterate (FUNCTOR fct) const
    {
        for (std::size_t i=0; i<SIZE; i++)  { fct (i, data[i]);  }
    }

    template<typename FUNCTOR>
    static void iterate (const Sequence& a, const Sequence& b, FUNCTOR fct)
    {
        for (std::size_t i=0; i<SIZE; i++)
        {
            fct (i, a.data[i], b.data[i]);
        }
    }

};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif /* _BPL_SEQUENCE_HPP_ */
