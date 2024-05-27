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
struct Array1
{
    USING(ARCH);

    using array_t = array<uint16_t,128>;

    auto operator() (const array_t& data, char coeff)
    {
        uint64_t sum = 0;  for (auto c : data)  {  sum += c;  }  return coeff*sum;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
