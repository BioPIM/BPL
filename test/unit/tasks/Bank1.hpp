////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>
#include <bpl/bank/BankChunk.hpp>

using namespace bpl::core;
using namespace bpl::bank;

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Bank1 : Task<ARCH>
{
    USING(ARCH);

    using seqarray_t = array<Sequence<32>,64>;

    auto operator() (seqarray_t& sequences, pair<int,int> range)
    {
        if (this->tuid()== 0)
        {
            for (auto&& seq : slice(sequences,range))
            {
                seq.iterate ([] (int i, uint8_t c)  { printf ("%c", c); });  printf("\n");
            }
        }

        return 0;
    }
};
