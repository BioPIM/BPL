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
struct Compare1
{
    USING(ARCH);

    static const int NBREF  = 64;
    static const int NBQRY  = 64;

    static const int SEQLEN = 32;

    auto operator() (
        const BankChunk<ARCH,SEQLEN,NBREF>& ref,
        const BankChunk<ARCH,SEQLEN,NBQRY>& qry,
        pair<int,int> range,
        size_t threshold
    )
    {
        size_t nbFound = 0;

        //printf ("%4d  [%4d:%4d]\n", puid, range.first, range.second);

    for (int n=1; n<=20; n++)
    {
        for (auto&& s1 : ref)
        {
            for (auto&& s2 : slice(qry,range))
            {
                size_t common=0;
                size_t total =0;

                Sequence<SEQLEN>::iterate (s1,s2, [&] (int idx, auto a, auto b)
                {
                    common += a==b ? 1 : 0;
                    total  += 1;
                });

                if (100*common >= threshold * total)  {  nbFound ++;  }
            }
        }
    }

        return nbFound;
    }

    static auto reduce (size_t a, size_t b)  {  return a+b;  }
};
