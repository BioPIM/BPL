////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

using namespace bpl;

template<class ARCH>
struct Checksum4
{
    USING(ARCH);

    using buffer_t = array<uint32_t,64>;

    uint64_t operator() (const buffer_t& buffer,  const pair<int,int>& range)
    {
        return accumulate (slice(buffer,range), 0);
    }

    static size_t reduce (size_t a, size_t b)  { return a+b; }
};
