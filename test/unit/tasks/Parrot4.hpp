////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct SomeData
{
    int x;  int y; double z;  char t;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Parrot4 : bpl::core::Task<ARCH>
{
    USING(ARCH);

     auto operator() (
        const array<SomeData,8>& data,
        const array<char,8>&     other,
        uint16_t n,
        double   x,
        char     c
    )
    {
         if (this->tuid()==10000)
         {
             printf ("TASKLET[%4d]: n=%d  x=%lf  c=%c \n", this->tuid(), n, x, c);

             for (const auto& item : data)  {  printf ("data: %2d  %2d  %f  %c\n", item.x, item.y, item.z, item.t); }

             printf ("other: ");  for (const auto& c : other) {  printf ("%c", c); } printf ("\n");
         }

         return make_tuple (other,n, x, c);
    }
};
