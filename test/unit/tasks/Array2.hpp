////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: fdemoor
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Array2
{
    USING(ARCH);

    using array_t = array<uint16_t,128>;

    auto operator() (const array_t& arr, char coeff)
    {
        uint64_t sum = 0;
        auto data = arr.data();
        for (size_t i = 0; i < arr.size(); ++i) {
            sum += data[i];
        }
        return coeff*sum;
    }

    static auto reduce (uint64_t a, uint64_t b)  { return a+b; }
};
