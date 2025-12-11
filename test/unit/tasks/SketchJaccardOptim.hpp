////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct SketchJaccardOptim
{
    USING(ARCH);

    using hash_t   = uint32_t;
    using count_t  = uint16_t;

    auto operator() (vector_view<hash_t>& dbRef, vector_view<hash_t>& dbQry, size_t ssize)
    {
        vector<count_t> result;

        size_t nbSketchRef = dbRef.size() / ssize;
        size_t nbSketchQry = dbQry.size() / ssize;

        for (size_t idxSketchQry=0, offsetQry=0; idxSketchQry<nbSketchQry; idxSketchQry++, offsetQry+=ssize)
        {
            for (size_t idxSketchRef=0, offsetRef=0; idxSketchRef<nbSketchRef; idxSketchRef++, offsetRef+=ssize)
            {
                hash_t hashRef = dbRef [offsetRef+0];
                hash_t hashQry = dbQry [offsetQry+0];

                count_t count = 0;

                for (size_t r=0, q=0; r<ssize and q<ssize; )  // loop that compares two sketches
                {
                        if (hashRef < hashQry)   { ++r;   hashRef = dbRef[offsetRef+r]; }
                   else if (hashRef > hashQry)   { ++q;   hashQry = dbQry[offsetQry+q]; }
                   else
                   {
                       ++r; ++q; count++;
                       hashRef = dbRef[offsetRef+r];
                       hashQry = dbQry[offsetQry+q];
                   }
                }

                result.push_back (count);
            }
        }

        return result;
    }
};
