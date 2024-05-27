////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

template<class ARCH>
struct Checksum1
{
    USING(ARCH);

    size_t operator() (const string& label, const array<int,100>& buffer, pair<size_t,size_t> bounds)
    {
        size_t result = 0;
        for (auto x : buffer)  { result += x; }
        return result;
    }
};
