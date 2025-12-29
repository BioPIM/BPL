////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
// @description: The task gets a vector as input and just returns it.
// @benchmark-input: 2^n for n in range(19)
// @benchmark-split: no
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorSerialize
{
    struct config  {  static const bool VECTOR_SERIALIZE_OPTIM = true;  };

    USING(ARCH,config);

    using type = uint32_t;

    decltype(auto) operator() (vector<type>& v) const
    {
        return v;
    }
};
