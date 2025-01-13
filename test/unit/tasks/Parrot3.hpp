////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct MyData
{
    int x;  int y; double z;  char t;
    bool operator== (const MyData& other) const  { return x==other.x and y==other.y and z==other.z and t==other.t; }
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Parrot3 : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (
        const MyData& data,
        const pair<int,int>& coords,
        char c,
        tuple<char,int,double> t,
        char x,
        tuple<char,MyData> other
    )
    {
        printf ("TASKLET[%4d]:   {%d   %d  %f  %d}  [%d %d]  '%c'  { '%c' %d %f}  '%c'   {%d  {%d   %d  %f  %d}} \n",
            this->tuid(),
            data.x, data.y, data.z, data.t,
            coords.first, coords.second,
            c,
            get<0>(t), get<1>(t), get<2>(t),
            x,
            get<0>(other), get<1>(other).x, get<1>(other).y, get<1>(other).z, get<1>(other).t
        );

        return make_tuple (data, coords, c, t, x, other);
    }
};
