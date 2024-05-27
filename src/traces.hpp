////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#ifndef _BPL_TRACES_HPP_
#define _BPL_TRACES_HPP_

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

#ifdef DPU
    #define DEBUG_SERIALIZATION(a...)     //if (me()==0)  printf (a)
#else
    #define DEBUG_SERIALIZATION(a...)     //printf (a)
#endif

#define VERBOSE_SERIALIZATION(a...)   //printf (a)

#define DEBUG_LAUNCHER(a...)        //printf (a)
#define VERBOSE_LAUNCHER(a...)      //printf (a)

#define DEBUG_ARCH_UPMEM(a...)        //printf (a)
#define VERBOSE_ARCH_UPMEM(a...)      //printf (a)

#define DEBUG_SPLIT(a...)        //printf (a)
#define VERBOSE_SPLIT(a...)      //printf (a)

#ifdef DPU
    #define DEBUG_VECTOR(a...)        //if (me()==0)  printf (a)
    #define VERBOSE_VECTOR(a...)      //if (me()==0)  printf (a)
#else
    #define DEBUG_VECTOR(a...)        //printf (a)
    #define VERBOSE_VECTOR(a...)      //printf (a)
#endif

////////////////////////////////////////////////////////////////////////////////

#endif /* _BPL_TRACES_HPP_ */
