////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef _BPL_BANK_CHUNK_HPP_
#define _BPL_BANK_CHUNK_HPP_

#include <bpl/bank/Sequence.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace bank {
////////////////////////////////////////////////////////////////////////////////

template<class ARCH, int SEQSIZE=32, int SEQNB=64>
class BankChunk
{
public:

    USING(ARCH);

    BankChunk() {}

    template<typename GENERATOR>
    BankChunk (GENERATOR generator)
    {
        auto outBegin = this->begin();
        auto outEnd   = this->end();

        for ( ; outBegin!=outEnd; ++generator, ++outBegin)  {  *(outBegin) = *generator;  }
    }

    auto begin() const  { return sequences_.begin();  }
    auto end  () const  { return sequences_.end();    }

    auto begin() { return sequences_.begin();  }
    auto end  () { return sequences_.end();    }

    template<typename FUNCTOR>
    void iterate (FUNCTOR fct) const
    {
        auto it  = this->begin();
        auto end = this->end  ();
        for (std::size_t i=0; it!=end; ++it, ++i)  { fct (i, *it); }
    }

    size_t size() const { return sequences_.size();  }

    array<Sequence<SEQSIZE>,SEQNB> sequences_;
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

namespace bpl { namespace core {

    template<typename ARCHI, int SEQSIZE, int SEQNB>
    struct serializable<bpl::bank::BankChunk<ARCHI,SEQSIZE,SEQNB>>
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
}};

////////////////////////////////////////////////////////////////////////////////

#endif /* _BPL_BANK_CHUNK_HPP_ */
