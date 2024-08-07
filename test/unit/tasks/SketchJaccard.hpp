////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct SketchJaccard : bpl::core::Task<ARCH>
{
    USING(ARCH);

    using hash_t   = uint32_t;
    using count_t  = uint16_t;
    using sketch_t = array<hash_t,36>;

    vector<count_t> operator() (const vector<sketch_t>& dbRef, const vector<sketch_t>& dbQry)
    {
        vector<count_t> result;

        for (const sketch_t& sketchQry : dbQry)  // loop over sketches of the query
        {
            for (const sketch_t& sketchRef : dbRef)  // loop over sketches of the reference
            {
                count_t count = 0;

                hash_t hashRef = sketchRef[0];
                hash_t hashQry = sketchQry[0];

                for (size_t r=0, q=0; r<sketchRef.size() and q<sketchQry.size(); )  // loop that compares two sketches
                {
                        if (hashRef < hashQry)   { hashRef = sketchRef[++r]; }
                   else if (hashRef > hashQry)   { hashQry = sketchQry[++q]; }
                   else                          { hashRef = sketchRef[++r];  hashQry = sketchQry[++q];  count++; }
                }

                result.push_back (count);
            }
        }

        return result;
    }
};
