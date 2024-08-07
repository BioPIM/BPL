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
struct VectorMove2
{
    USING(ARCH);

    using result_t = vector<uint32_t>;

    result_t operator() (result_t& v1)
    {
        result_t v2;

        v2 = std::move(v1);

        return v2;
    }
};
