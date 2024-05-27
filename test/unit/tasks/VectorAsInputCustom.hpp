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
struct VectorAsInputCustom
{
    USING(ARCH);

    using type_t = uint32_t;

    struct MyData
    {
        MyData (uint8_t a, uint16_t b, uint32_t c, uint64_t d, double e)  : a(a), b(b), c(c), d(d), e(e) {}

        MyData () {}

        uint8_t  a;
        uint16_t b;
        uint32_t c;
        uint64_t d;
        double   e;
    };

    using vector_t = vector<MyData>;

    auto operator() (vector_t& input)
    {
        uint64_t nberrors = 0;

        auto trim = [] (uint32_t i, auto x)
        {
            return i & std::numeric_limits<decltype(x)>::max();
        };

        uint32_t i=1;
        for (auto entry : input)
        {
            nberrors += entry.a != trim(1*i,entry.a) ? 1 : 0;
            nberrors += entry.b != trim(2*i,entry.b) ? 1 : 0;
            nberrors += entry.c != trim(3*i,entry.c) ? 1 : 0;
            nberrors += entry.d != trim(4*i,entry.d) ? 1 : 0;
            nberrors += entry.e != 5*i ? 1 : 0;

            i++;
        }

        return nberrors;
    }
};
