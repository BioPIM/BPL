////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/utils/serialize.hpp>
#include <bpl/utils/split.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** Sequence holding a fixed size of characters.
 *
 * It provides an API for iterating the characters.
 *
 * \param S: the number of characters of the sequence.
 */
template<std::size_t S>
struct Sequence
{
    /** Type of the characters of the sequence */
    using data_t = uint8_t;

    /** Number of characters. */
    static const std::size_t SIZE = S;

    /** Array holding the characters of the sequence. */
    data_t data [SIZE];

    static constexpr data_t nucleotids[] = {'A', 'C', 'T', 'G' };

    /** \return the number of characters. */
    size_t size() const { return SIZE; }

    /** Iterate the sequence through a functor. The functor will
     * be called with (1) the index of the character and (2) the
     * character itself. */
    template<typename FUNCTOR>
    void iterate (FUNCTOR fct) const
    {
        for (std::size_t i=0; i<SIZE; i++)  { fct (i, data[i]);  }
    }

    /** Iterate in parallel two sequences through a functor.
     * \param a: the first sequence to be iterated.
     * \param b: the second sequence to be iterated.
     * \param fct: the functor that will be called with each couple of characters
     * from a and b.
     *  */
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
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
