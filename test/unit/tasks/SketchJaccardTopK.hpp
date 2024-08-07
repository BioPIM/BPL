////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct SketchJaccardTopK : bpl::core::Task<ARCH>
{
    USING(ARCH);

    using hash_t   = uint32_t;
    using count_t  = uint16_t;

    auto operator() (const vector_view<hash_t>& dbRef, const vector_view<hash_t>& dbQry, size_t ssize)
    {
        vector<count_t> result;

        auto refStart = dbRef.begin();
        auto qryStart = dbQry.begin();

        size_t nbSketchRef = dbRef.size() / ssize;
        size_t nbSketchQry = dbQry.size() / ssize;

        for (size_t idxSketchQry=0, offsetQry=0; idxSketchQry<nbSketchQry; idxSketchQry++, offsetQry+=ssize)
        {
            for (size_t idxSketchRef=0, offsetRef=0; idxSketchRef<nbSketchRef; idxSketchRef++, offsetRef+=ssize)
            {
                count_t count = 0;

#define VERSION 2

#if VERSION==1  // 147.3 s   (#ref=10  #qry=700)

                size_t r = offsetRef;
                size_t q = offsetQry;

                size_t rmax = offsetRef + ssize;
                size_t qmax = offsetQry + ssize;

                hash_t hashRef = dbRef [r];
                hash_t hashQry = dbQry [q];

                while (true)  // loop that compares two sketches
                {
                        if (hashRef < hashQry)   { if (++r<rmax) {  hashRef = dbRef[r]; } else { break; } }
                   else if (hashRef > hashQry)   { if (++q<qmax) {  hashQry = dbQry[q]; } else { break; } }
                   else
                   {
                       count++;
                       if (++r<rmax) {  hashRef = dbRef[r]; } else { break; }
                       if (++q<qmax) {  hashQry = dbQry[q]; } else { break; }
                   }
                }
#endif

#if VERSION==2  // 142.7 s   (#ref=16  #qry=1200)

                auto refBegin = refStart + offsetRef;
                auto refEnd   = refBegin + ssize;

                auto qryBegin = qryStart + offsetQry;
                auto qryEnd   = qryBegin + ssize;

                while (true)  // loop that compares two sketches
                {
                        if (*refBegin < *qryBegin)   {   if (++refBegin == refEnd)  { break; }   }
                   else if (*refBegin > *qryBegin)   {   if (++qryBegin == qryEnd)  { break; }   }
                   else
                   {
                       count++;
                       if (++refBegin == refEnd)  { break; }
                       if (++qryBegin == qryEnd)  { break; }
                   }
                }
#endif

#if VERSION==3  // 150.5 s   (#ref=16  #qry=1200)

                auto refBegin = refStart + offsetRef;
                auto refEnd   = refBegin + ssize;

                auto qryBegin = qryStart + offsetQry;
                auto qryEnd   = qryBegin + ssize;

                for ( ; refBegin!=refEnd and qryBegin!=qryEnd; )  // loop that compares two sketches
                {
                        if (*refBegin < *qryBegin)   { ++refBegin; }
                   else if (*refBegin > *qryBegin)   { ++qryBegin; }
                   else
                   {
                       count++;  ++refBegin; ++qryBegin;
                   }
                }
#endif

#if VERSION==4  // 216.7 s   (#ref=10  #qry=700)

                for (size_t r=0, q=0; r<ssize and q<ssize; )  // loop that compares two sketches
                {
                    hash_t hashRef = dbRef [r];
                    hash_t hashQry = dbQry [q];

                        if (hashRef < hashQry)   { r++; }
                   else if (hashRef > hashQry)   { q++; }
                   else
                   {
                       count++;  r++; q++;
                   }
                }

#endif

#if VERSION==5  // 310.7 s   (#ref=10  #qry=700)

                for (size_t r=0, q=0; r<ssize and q<ssize; )  // loop that compares two sketches
                {
                    int32_t diff = dbRef [r] - dbQry [q];

                    r     += diff > 0 ? 0 : 1;
                    q     += diff > 0 ? 1 : 0;
                    count += diff==0;
                }
#endif

                result.push_back (count);
            }

            // We notify the progression of the task
            this->notify (idxSketchQry,nbSketchQry);
        }

        return result;
    }
};
