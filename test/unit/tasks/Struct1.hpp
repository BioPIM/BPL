////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class ARCH>
struct Struct1
{
    USING(ARCH);

    using type = uint8_t;

    struct foo
    {
        type a1;  type a2; type a3; type a4; type a5;  type a6;  type a7;  type a8;
    };

    auto operator() (foo const& x) const {  return x;  }
};
