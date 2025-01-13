////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/bank/BankChunk.hpp>

using namespace bpl;

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Compare2
{
    USING(ARCH);

    static const int NBREF  = 64;
    static const int SEQLEN = 32;

    auto operator() (
        const BankChunk<ARCH,SEQLEN,NBREF>& ref,
        pair<uint32_t,uint32_t> range,
        size_t threshold
    )
    {
        size_t nbFound = 0;

        return nbFound;
    }

    static auto reduce (size_t a, size_t b)  {  return a+b;  }
};
