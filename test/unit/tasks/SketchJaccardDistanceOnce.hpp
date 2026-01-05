////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
// @description: Computes Jaccard distances between two sketches.
// @remark: we first create a result vector of N items and populate it with []
// for adding an item.
// @benchmark-input: 2^n for n in range(17,20)
// @benchmark-split: yes (dbRef)
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct SketchJaccardDistanceOnce : bpl::Task<ARCH>
{
    struct config  {
        static const int  VECTOR_MEMORY_SIZE_LOG2 = 10;
        static const bool VECTOR_SERIALIZE_OPTIM = true;
    };

    USING(ARCH,config);

    using hash_t   = uint32_t;
    using count_t  = uint16_t;

    auto operator() (
        once<vector_view<hash_t>const&> dbRefOnce,
        vector_view<hash_t>const& dbQry,
        size_t ssize
    )
    {
        auto& dbRef = *dbRefOnce;

        auto refStart = dbRef.begin();
        auto qryStart = dbQry.begin();

        size_t nbSketchRef = dbRef.size() / ssize;
        size_t nbSketchQry = dbQry.size() / ssize;

        vector<count_t> result (nbSketchRef*nbSketchQry);

        size_t k=0;
        for (size_t idxSketchQry=0, offsetQry=0;
            idxSketchQry<nbSketchQry;
            idxSketchQry++, offsetQry+=ssize)
        {
            for (size_t idxSketchRef=0, offsetRef=0;
                idxSketchRef<nbSketchRef;
                idxSketchRef++, offsetRef+=ssize)
            {
                count_t count = 0;

                auto refBegin = refStart + offsetRef;
                auto refEnd   = dbRef.end() - (dbRef.size()-(offsetRef + ssize));

                auto qryBegin = qryStart + offsetQry;
                auto qryEnd   = dbQry.end() - (dbQry.size()-(offsetQry + ssize));

                while (true)
                {
                   if (*refBegin < *qryBegin)   {
                       if (++refBegin == refEnd)  { break; }
                   }
                   else if (*refBegin > *qryBegin)   {
                       if (++qryBegin == qryEnd)  { break; }
                   }
                   else
                   {
                       count++;
                       if (++refBegin == refEnd)  { break; }
                       if (++qryBegin == qryEnd)  { break; }
                   }
                }

                result[k++] = count;
            }
        }

        return result;
    }
};
