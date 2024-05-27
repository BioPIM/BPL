////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>
#include <bpl/utils/vector.hpp>

////////////////////////////////////////////////////////////////////////////////

#ifdef DPU
extern bpl::arch::MRAM::Allocator<false> __MRAM_Allocator_nolock__;
#endif

////////////////////////////////////////////////////////////////////////////////

class Allocator
{
public:
    using address_t = uint64_t;

//    static Allocator& singleton () {  static Allocator instance; return instance; }

    static address_t write (void* src, size_t n)
    {
        address_t result = 0;
#ifdef DPU
        result = __MRAM_Allocator_nolock__.write (src, n);
#endif
//        nbwrites_++;
        return result;
    };

    static address_t* read (address_t src, void* tgt, size_t n)
    {

#ifdef DPU
        __MRAM_Allocator_nolock__.read (src, tgt, n);
#endif
//        nbreads_++;
        return (address_t*) tgt;
    };

    static constexpr bool is_freeable = false;

    static void free (address_t a)
    {
        // not really a free, isn't it ?
    }

    static void debug (FILE* out=stdout)
    {
#ifdef DPU
//        if (me()==0)   printf ("[ALLOCATOR: #write=%d  #read=%d\n", nbwrites_, nbreads_);
#endif
    }

    static void reset()
    {
#ifdef DPU
        __MRAM_Allocator_nolock__.reset();
#endif
    }

    static void reset_stats()
    {
//        nbwrites_ = nbreads_ = 0;
    }

//    size_t nbwrites_ = 0;
//    size_t nbreads_  = 0;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct Vector2 : bpl::core::Task<ARCH>
{
    USING(ARCH);

    uint32_t operator() (uint32_t N, int K)
    {
        using type = uint32_t;

        if (this->tuid()==0)  {  Allocator::reset();  }

        //uint64_t truth = uint64_t(N)*(N+1)/2;  // need to cast N

        if (this->tuid()==0)
        {
            for (int k=1; k<=K; k++)
            {
                bpl::core::vector <type, Allocator, 8, 3, 10> vec;

                for (type i=1; i<=N; i++)  {  vec.push_back (i);  }

                Allocator::reset();
            }

//            Allocator::singleton().debug();
//            Allocator::singleton().reset_stats();
//            printf ("\n");
//
//            uint64_t checksum = 0;
//            vec.iterate ([&] (uint32_t x)  {  checksum += x;  });

//            Allocator::singleton().debug();
//            Allocator::singleton().reset_stats();
//            printf ("\n");

//            printf ("sizeof(vec): %d   max: %ld  size: %ld   N=%d  [truth,checksum]: [%ld,%ld] -> %s \n",
//                sizeof(vec), vec.max_size(), vec.size(), N, truth, checksum, truth==checksum ? "OK" : "KO"
//            );
        }

        return 0;
    }
};
