////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

struct MyData2
{
#ifdef DPU
    static const char* line() { return "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD  > "; }
#else
    static const char* line() { return "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH  > "; }

#endif

    MyData2 ()   { printf ("%s [MyData2]  default constructor  %p\n", line(), this);  }
    ~MyData2 ()  { printf ("%s [MyData2]  destructor %p\n", line(), this);  }

    MyData2 (const  MyData2& other) : x(other.x), y(other.y), z(other.z), t(other.t)
    { printf ("%s [MyData2]  copy constructor %p \n", line(), this);  }

#if 1

    MyData2 (MyData2&& other)
     : x(std::move(other.x)), y(std::move(other.y)), z(std::move(other.z)), t(std::move(other.t))
    {
        printf ("%s [MyData2]  move constructor %p\n", line(), this);
    }

    MyData2& operator= (MyData2&& other)
    {
        printf ("%s [MyData2]  move assignment %p\n", line(), this);
        std::swap (x,other.x);
        std::swap (y,other.y);
        std::swap (z,other.z);
        std::swap (t,other.t);
        return *this;
    }

#endif

//    MyData2 (const  MyData2& other)  { printf ("-----------------------------------------------------------------------------------------------------> [MyData2]  copy constructor\n");  }

    explicit MyData2 (int x, int  y, double z, char t) : x(x), y(y), z(z), t(t)
    {
        printf ("%s [MyData2]  constructor %p\n", line(), this);
    }

    //
//    MyData (const MyData& other)  { printf ("-------------> [MyData]  copy constructor\n");  }
//
//    MyData& operator= (const MyData&& other)  { printf ("-------------> [MyData]  assigment\n");  }


    int x;  int y; double z;  char t;
    bool operator== (const MyData2& other) const  { return x==other.x and y==other.y and z==other.z and t==other.t; }
};

template<class ARCH>
struct Parrot2 : bpl::core::Task<ARCH>
{
    auto operator() (const MyData2& data)
    {
        printf ("TASKLET[%3d]:   {%d   %d  %f  %d} \n",  this->tuid(),  data.x, data.y, data.z, data.t);

        return std::make_tuple (data);
    }
};
