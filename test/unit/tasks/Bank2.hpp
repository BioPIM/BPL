////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>
#include <bpl/bank/BankChunk.hpp>

using namespace bpl;

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Bank2 : Task<ARCH>
{
    USING(ARCH);

    static const int SEQLEN = 8;
    static const int SEQNB  = 32;

    using seqarray_t = array<Sequence<SEQLEN>,SEQNB>;

    using result_t = std::tuple <uint32_t, uint32_t, uint32_t, uint32_t>;

    result_t operator() (seqarray_t& sequences, std::pair<int,int> range)
    {
        uint32_t A=0, C=0, G=0, T=0;

        // We iterate each sequence of the array
        for (const Sequence<SEQLEN>& seq : slice(sequences,range))
        {
            // We iterate each character of the sequence
            seq.iterate ([&] (int i, uint8_t x)
            {
                switch (x)
                {
                    case 'A':  A++;   break;
                    case 'C':  C++;   break;
                    case 'G':  G++;   break;
                    case 'T':  T++;   break;
                }
            });
        }

        return result_t {A,C,G,T};
    }
};
