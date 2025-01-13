////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

struct MyData2
{
    int x;  int y; double z;  char t;
    bool operator== (const MyData2& other) const  { return x==other.x and y==other.y and z==other.z and t==other.t; }
};

template<class ARCH>
struct Parrot2 : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (const MyData2& data)
    {
        printf ("TASKLET[%3d]:   {%d   %d  %f  %d} \n",  this->tuid(),  data.x, data.y, data.z, data.t);

        return std::make_tuple (data);
    }
};
