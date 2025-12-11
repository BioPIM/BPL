////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: fdemoor
////////////////////////////////////////////////////////////////////////////////

#pragma once

struct MyLong {
    uint64_t value;
};

template <class ARCH>
struct SplitMyLong : bpl::Task<ARCH>
{
    USING(ARCH);

    uint64_t operator()(const MyLong x) {
        return x.value;
    }
};
