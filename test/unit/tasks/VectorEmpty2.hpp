////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////

template<class ARCH>
struct DataVectorEmpty
{
    USING(ARCH);

    vector<int> a;
    vector<int> b;
    vector<int> c;
};


template<class ARCH>
struct VectorEmpty2
{
    USING(ARCH);

    auto operator() (DataVectorEmpty<ARCH> const& data)
    {
        return 0;
    }
};
