////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef _BPL_BANKFASTA_HPP_
#define _BPL_BANKFASTA_HPP_

#include <string>
#include <bpl/bank/Sequence.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace bank {
////////////////////////////////////////////////////////////////////////////////

template<class ARCH, int SEQSIZE=32>
class BankFasta
{
    struct iterator;

public:

    BankFasta (const std::string& uri) : uri_(uri)  {}

    iterator begin() const { return iterator (uri_); }
    iterator end  () const { return iterator ();     }

private:

    struct iterator
    {
        iterator (const std::string& uri="") : uri_(uri) {}

        bool operator!=(const iterator& other)  { return false; }
        const Sequence<SEQSIZE>& operator*() const { return currentSeq_; }
        iterator& operator++() { return *this; }

        std::string uri_;
        Sequence<SEQSIZE> currentSeq_;
    };

    std::string uri_;
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif /* _BPL_BANKFASTA_HPP_ */
