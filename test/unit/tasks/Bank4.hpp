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
struct Bank4 : Task<ARCH>
{
    USING(ARCH);

    auto operator() (const BankChunk<ARCH>& bank)
    {
        if (this->tuid() != 0)  { return 0; }

        bank.iterate ([] (int idx, const auto& seq)
        {
            seq.iterate ([] (int i, char c)  {  printf ("%c",c);  });  printf ("\n");
        });

        return 0;
    }
};
