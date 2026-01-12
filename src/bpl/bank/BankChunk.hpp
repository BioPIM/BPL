////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/bank/Sequence.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** Class that emulates a genomics bank, ie. something holding sequences of
 * characters.
 *
 * Such a bank is created during a call to the constructor by using a generator
 * object that will generates sequences to be kept in array.
 *
 * Then, an API for iterating the sequence is provided, either through begin/end
 * methods, or by a call to a functor called for each sequence by an 'iterate' function.
 *
 * This class is not really designed for real life use case but rather for unit tests.
 *
 * \param ARCH: architecture to be used. It might be odd to have an architecture here
 * but it is important when this class is used in a UPMEM context.
 * \param SEQSIZE: number of characters for a sequence.
 * \param SEQNB: number of sequences in the bank.
 */
template<class ARCH, int SEQSIZE=32, int SEQNB=64>
class BankChunk
{
public:

    /** This class is not parseable, which means that its attributes won't be
     * recursively analyzed by some type traits (see bpl::CounterTrait for instance)
     */
    static constexpr bool parseable = false;

    USING(ARCH);

    /** Default constructor. */
    BankChunk() {}

    /** Create a bank from a given sequence generator.
     * \param generator: the sequences generator.
     */
    template<typename GENERATOR>
    BankChunk (GENERATOR generator)
    {
        auto outBegin = this->begin();
        auto outEnd   = this->end();

        for ( ; outBegin!=outEnd; ++generator, ++outBegin)  {  *(outBegin) = *generator;  }
    }

    /** \return a iterator starting at the first sequence. */
    auto begin() const  { return sequences_.begin();  }

    /** \return an ending iterator. */
    auto end  () const  { return sequences_.end();    }

    /** \return a iterator starting at the first sequence. */
    auto begin() { return sequences_.begin();  }

    /** \return an ending iterator. */
    auto end  () { return sequences_.end();    }

    /** Iterate the bank through a provided functor.
     * \param fct: the functor called with (1) the index of the sequence
     * and (2) the sequence itself.
     */
    template<typename FUNCTOR>
    void iterate (FUNCTOR fct) const
    {
        auto it  = this->begin();
        auto end = this->end  ();
        for (std::size_t i=0; it!=end; ++it, ++i)  { fct (i, *it); }
    }

    /** Number of sequences in the bank. */
    size_t size() const { return sequences_.size();  }

    array<Sequence<SEQSIZE>,SEQNB> sequences_;
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////

namespace bpl
{
    template<typename ARCHI, int SEQSIZE, int SEQNB>
    struct serializable<bpl::BankChunk<ARCHI,SEQSIZE,SEQNB>>
    {
        // we tell that our structure can be serialized
        static constexpr int value = true;

        template<class ARCH, class BUFITER, int ROUNDUP, typename T, typename FCT>
        static auto iterate (bool transient, int depth, const T& t, FCT fct, void* context=nullptr)
        {
            Serialize<ARCH,BUFITER,ROUNDUP>::iterate (transient, depth+1, t.sequences_, fct, context);
        }

        template<class ARCH, class BUFITER, int ROUNDUP, typename T>
        static auto restore (BUFITER& it, T& result)
        {
            Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, result.sequences_);
        }
    };
};

