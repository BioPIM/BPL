////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

using namespace bpl::core;

template<class ARCH>
struct Checksum5
{
    USING(ARCH);

    using buffer_t = array<uint32_t,256>;

    auto operator() (const buffer_t& buffer)
    {
        pair<int,int> range (0, buffer.size());

        return accumulate (slice(buffer,range), uint64_t(0));
    }

    static size_t reduce (size_t a, size_t b)  { return a+b; }
};
