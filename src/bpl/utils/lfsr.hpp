////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_UTILS_LFSR_HPP__
#define __BPL_UTILS_LFSR_HPP__

#include <cstdint>

////////////////////////////////////////////////////////////////////////////////
inline uint32_t lfsr_next (uint32_t next_val, uint8_t n)
{
   static uint32_t taps[] = {
       0,
       /*   1 */  0x00000001,    /*   2 */  0x00000003,    /*   3 */  0x00000006,    /*   4 */  0x0000000c,
       /*   5 */  0x00000014,    /*   6 */  0x00000030,    /*   7 */  0x00000060,    /*   8 */  0x000000b8,
       /*   9 */  0x00000110,    /*  10 */  0x00000240,    /*  11 */  0x00000500,    /*  12 */  0x00000829,
       /*  13 */  0x0000100d,    /*  14 */  0x00002015,    /*  15 */  0x00006000,    /*  16 */  0x0000d008,
       /*  17 */  0x00012000,    /*  18 */  0x00020400,    /*  19 */  0x00040023,    /*  20 */  0x00090000,
       /*  21 */  0x00140000,    /*  22 */  0x00300000,    /*  23 */  0x00420000,    /*  24 */  0x00e10000,
       /*  25 */  0x01200000,    /*  26 */  0x02000023,    /*  27 */  0x04000013,    /*  28 */  0x09000000,
       /*  29 */  0x14000000,    /*  30 */  0x20000029,    /*  31 */  0x48000000,    /*  32 */  0x80200003
   };

   bool lsb = (next_val & 1) == 1;
   next_val = next_val >> 1;
   if (lsb)  {  next_val ^= taps[n];  }
   return next_val;
}

////////////////////////////////////////////////////////////////////////////////
#endif /* __BPL_UTILS_LFSR_HPP__ */
