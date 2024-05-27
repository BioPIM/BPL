////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorSwap
{
    USING(ARCH);

    using result_t = vector<uint32_t>;

    result_t operator() (uint32_t n)
    {
        result_t result;

        // We fill the vector with integer 1..2n
        for (uint32_t i=1; i<=2*n; i++)  {  result.push_back (i);  }

        // We reverse the vector by using 'swap'
        for (uint32_t i=0; i<n; i++)
        {
            swap (result[i], result[result.size()-1-i]);
        }

        // We return the result.
        return result;
    }
};
