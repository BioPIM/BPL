////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

using namespace bpl;

template<class ARCH>
struct Checksum3
{
    USING(ARCH);

    using buffer_t = array<uint32_t,64>;

    auto operator() (const buffer_t& buffer,  const pair<int,int>& range)
    {
        uint64_t result = 0;
        for (auto x : slice(buffer,range))  { result += x; }
        return result;
    }

    static size_t reduce (size_t a, size_t b)  { return a+b; }
};
