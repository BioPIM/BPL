////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <bpl/core/Launcher.hpp>
#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>
#include <bpl/arch/ArchDummy.hpp>
#include <bpl/utils/split.hpp>

using namespace bpl::core;
using namespace bpl::arch;

#include <tasks/Vector1.hpp>
#include <tasks/Vector2.hpp>
#include <tasks/Vector3.hpp>
#include <tasks/VectorCheck.hpp>
#include <tasks/VectorAsInput.hpp>
#include <tasks/VectorAsInput2.hpp>
#include <tasks/VectorAsInput3.hpp>
#include <tasks/VectorAsInputSplit.hpp>
#include <tasks/VectorAsInputCustom.hpp>
#include <tasks/VectorChecksum.hpp>
#include <tasks/VectorAsOutputUint8.hpp>
#include <tasks/VectorAsOutputUint16.hpp>
#include <tasks/VectorAsOutputUint32.hpp>
#include <tasks/VectorInputOutput.hpp>
#include <tasks/VectorInputOutput2.hpp>
#include <tasks/VectorOfVectors.hpp>
#include <tasks/VectorFlush.hpp>
#include <tasks/VectorSwap.hpp>
#include <tasks/VectorAdd.hpp>
#include <tasks/VectorMove1.hpp>
#include <tasks/VectorMove2.hpp>
#include <tasks/VectorEmplaceBack.hpp>

#include <Runner.hpp>

//////////////////////////////////////////////////////////////////////////////
struct config
{
    struct upmem
    {
        using arch_t     = ArchUpmem;
        using resource_t = ArchUpmem::DPU;
        using unit_t     = ArchUpmem::Tasklet;
    };

    struct multicore
    {
        using arch_t     = ArchMulticore;
        using resource_t = ArchMulticore::Thread;
        using unit_t     = ArchMulticore::Thread;
    };

    using types = std::tuple<multicore,upmem>;

    template<template <typename> typename FCT>
    static auto run ()
    {
        std::apply ([](auto...arg)  {  (FCT<decltype(arg)>() (),...);  }, types{});
    }
};

//////////////////////////////////////////////////////////////////////////////
static int getNbUnits()
{
    if (getenv("NB_RANKS"))  {  return atoi(getenv("NB_RANKS")); }
    if (getenv("NB_DPU"))    {  return atoi(getenv("NB_DPU"));   }
    return 1;
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Vector1", "[Vector]" )
{
    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1));

    uint32_t nbitems = 1300*4*8 + 20;

    auto result = launcher.run<Vector1> (nbitems);

    for (auto res : result)
    {
        REQUIRE (res == nbitems);
    }
}

//////////////////////////////////////////////////////////////////////////////
#if 0  // SHOULD BE MOVED IN BENCHMARK TESTS
TEST_CASE ("Vector2", "[Vector]" )
{
    for (int i=1; i<=23; i++)
    {
        Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1));

        uint32_t nbitems = 1UL << i;

        int K = 1UL << (23-i + 1);

        auto result = launcher.run<Vector2> (nbitems, K);

        auto& stats = launcher.getStatistics();

        double ts = stats.getTiming("run/launch") / double(K);

        printf ("%4d  %8d  %10.7f  %8.5f \n",  i, nbitems, ts, (ts>0 ? double(nbitems)/ts : 0)/1000000.0);
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////
#if 0  // SHOULD BE MOVED IN BENCHMARK TESTS
TEST_CASE ("Vector3", "[Vector]" )
{
    using type_t = Vector3<ArchMulticore>::type_t;

    uint64_t sizeMRAM = 1ULL << 26;
    uint64_t nbitems  = ((90*sizeMRAM)/100) / NR_TASKLETS / sizeof(type_t);

    for (int nbtasks=1; nbtasks<=NR_TASKLETS; nbtasks++)
    {
        Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1));

        auto result = launcher.run<Vector3> (nbtasks, nbitems);

        auto& stats = launcher.getStatistics();

        double rate = double(nbitems) / stats.getTiming("run/launch") / 1000000.0;
        printf ("%2d  %.5f\n", nbtasks, rate);
    }
}
#endif

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorCheck", "[Vector]" )
{
    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1), false /* no traces*/, true /* reset between calls */);

    auto fct = [&] (uint32_t nbitems)
    {
        auto result = launcher.run<VectorCheck> (nbitems);

        size_t i=0;

        for (auto&& res : result)
        {
            uint32_t actualNbitems = nbitems + i;

            uint64_t truth = uint64_t(actualNbitems)*(actualNbitems+1)/2;

            // printf ("nb: [%ld,%ld]  truth: [%ld,%ld]  \n",  std::get<0>(res), actualNbitems,  std::get<1>(res), truth);

            REQUIRE (std::get<0>(res) == actualNbitems);
            REQUIRE (std::get<1>(res) == truth);

            i++;
        }
    };

    for (uint32_t nbitems : { 10, 100, 1000, 10000, 100000} )  { fct (nbitems); }
    for (uint32_t nbitems=1; nbitems<=1000; nbitems++)         { fct (nbitems); }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorAsInput", "[Vector]" )
{
    Launcher<ArchUpmem> launcher (128_dpu, false/* traces yes/no */);

    //std::vector<uint32_t> v {1,1,2,3,5,8,13};
    std::vector<uint32_t> v;  for (size_t i=1; i<=40*1000; i++)  { v.push_back(i); }

    uint64_t truth = std::accumulate (v.begin(), v.end(), uint64_t(0));  // uint64_t cast mandatory for 0
    auto result = launcher.run<VectorAsInput> (v);

    REQUIRE (result.size() == launcher.getProcUnitNumber());

    uint64_t i = 0;
    for (auto res : result)
    {
        REQUIRE (res == truth+i);
        i++;
    }

    //launcher.getStatistics().dump (true);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorAsInput2", "[Vector]" )
{
    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(3), false /* traces yes/no */);

    std::vector<uint32_t> v1 { 0, 10, 20, 30, 40};
    std::vector<uint32_t> v2 {0,1,2,3,4,5,6,7,8,9};

    auto result = launcher.run<VectorAsInput2> (v1,v2);

    size_t nb = 10*v1.size();

    uint64_t truth = nb*(nb-1)/2;

    REQUIRE (result/launcher.getProcUnitNumber() == truth);

    //launcher.getStatistics().dump (true);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorAsInput3", "[Vector]" )
{
    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1), false /* traces yes/no */);

    size_t nb = 10;

    uint64_t s1=0;
    std::vector<uint32_t> v1;  for (size_t i=0; i<nb; i++)  { v1.push_back(2*i+0); s1+=v1.back(); }

    uint64_t s2=0;
    std::vector<uint32_t> v2;  for (size_t i=0; i<nb; i++)  { v2.push_back(2*i+1); s2+=v2.back(); }

    auto result = launcher.run<VectorAsInput3> (v1,v2);

    uint64_t truth = nb*(s1+s2);

    REQUIRE (result/launcher.getProcUnitNumber() == truth);  // no split -> must divide here

    //launcher.getStatistics().dump (true);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorAsInputSplit", "[Vector]" )
{
    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(getNbUnits()), false/* traces yes/no */);

    size_t nbitems = 2*64*1000*1000;
    uint64_t truth = 0;

    std::vector<uint32_t> v;
    for (size_t i=1; i<=nbitems; i++)
    {
        uint32_t val = 3*i;
        v.push_back(val);
        truth += val;
    }

    auto result = launcher.run<VectorAsInputSplit> (split(v), 100);

    REQUIRE (truth==result);

    //printf ("#items: %ld  result: %ld   truth: %ld  -> status: %s\n", nbitems, result, truth, truth==result ? "OK" : "KO");
    //launcher.getStatistics().dump (true);
}

//////////////////////////////////////////////////////////////////////////////
template<template<typename> class TASK>
auto VectorInputOutput_aux (const std::vector<size_t> sizes)
{
    for (size_t nb : sizes)
    {
        std::vector<uint32_t> v;  for (size_t i=1; i<=nb; i++)  { v.push_back(i); }

        for ( [[maybe_unused]] auto& [stats,result] : Runner<>::run<TASK> (v))
        {
            for (auto& res : result)
            {
                REQUIRE (res.size() == v.size());

                for (size_t i=0; i<v.size(); i++)
                {
                    //printf ("==> %d %d \n", 2*v[i], res[i]);
                    REQUIRE (2*v[i] == res[i]);
                }
            }
        }
    }
}

TEST_CASE ("VectorInputOutput", "[Vector]" )
{
    VectorInputOutput_aux <VectorInputOutput > (std::vector<size_t> {1,10,100,1000});
    VectorInputOutput_aux <VectorInputOutput2> (std::vector<size_t> {1,10,100,1000});
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorAsInputCustom", "[Vector]" )
{
    using type_t = VectorAsInputCustom<ArchUpmem>::type_t;
    using data_t = VectorAsInputCustom<ArchUpmem>::MyData;

    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1), false /* traces yes/no */);

    std::vector<data_t> v;
    for (type_t i=1; i<=100; i++)  { v.push_back (data_t (i, 2*i, 3*i, 4*i, 5.0*i)); }

    auto result = launcher.run<VectorAsInputCustom> (v);

    for (auto res : result)  { REQUIRE (res==0); }
}

//////////////////////////////////////////////////////////////////////////////

template<typename CONFIG, typename LAUNCHER>
void VectorChecksum_aux_aux (LAUNCHER& launcher, size_t nbItems)
{
    uint64_t truth = 0;
    std::vector<uint32_t> v;  for (size_t i=1; i<=nbItems; i++)  { v.push_back(i); truth+=v.back(); }

    auto checksum = launcher.template run<VectorChecksum> (split (v));

    //printf ("n: %6ld  checksum: %10ld   truth: %10ld\n", v.size(), checksum, truth);

    REQUIRE (checksum == truth);
}

template<typename CONFIG>
struct VectorChecksum_aux
{
    auto operator() ()
    {
        typename CONFIG::resource_t resources {4};
        Launcher<typename CONFIG::arch_t> launcher (resources, false/* traces yes/no */);

        size_t n = launcher.getProcUnitNumber();


        // We test a continuous range of integers
        for (size_t i=n; i<=2*n; i++)  {  VectorChecksum_aux_aux<CONFIG> (launcher, i);  }

        // We test some powers of 2
        for (size_t n=10; n<=17; n++)  {  VectorChecksum_aux_aux<CONFIG> (launcher, size_t(1)<<n);  }
    }
};

TEST_CASE ("VectorChecksum", "[Vector]" )
{
    config::run<VectorChecksum_aux> ();
}

//////////////////////////////////////////////////////////////////////////////
template<typename LAUNCHER>
void VectorChecksumLevels_aux (LAUNCHER&& launcher, size_t nbItems)
{
    uint64_t truth = 0;  std::vector<uint32_t> v;  for (size_t i=1; i<=nbItems; i++)  { v.push_back(i); truth+=v.back(); }

    REQUIRE (launcher.template run<VectorChecksum> (split<ArchUpmem::DPU>     (v)) == truth*NR_TASKLETS);
    REQUIRE (launcher.template run<VectorChecksum> (split<ArchUpmem::Tasklet> (v)) == truth);
}

TEST_CASE ("VectorChecksumLevels", "[Vector]" )
{
    for (size_t nbDPU : {1, 2, 3, 5, 8, 13, 21, 34, 55, 89})
    {
        Launcher<ArchUpmem> launcher (ArchUpmem::DPU(nbDPU), false /* traces yes/no */);

        for (size_t nbItems=1; nbItems<=200; nbItems++)
        {
            // printf ("nbDPU: %3ld   nbItems: %3ld \n", nbDPU, nbItems);
            VectorChecksumLevels_aux (launcher, nbItems);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

template<template <typename> typename  TASK, typename PROC_UNIT, typename ...ARGS>
void VectorAsOutput_aux_aux (PROC_UNIT pu, size_t nbItems, ARGS... args)
{
    using arch_t = typename PROC_UNIT::arch_t;

    Launcher<arch_t> launcher (pu, args...);

    size_t actualNbItems = nbItems / sizeof(typename TASK<arch_t>::type_t) / NR_TASKLETS;

    auto result  = launcher.template run<TASK> (actualNbItems);

//    static_assert (std::is_same_v <
//        decltype(result),
//        std::vector<std::vector<typename TASK<arch_t>::type_t>>
//    >);

    size_t i = 0;
    for (const auto& res : result)
    {
        REQUIRE (res.size() == actualNbItems+i+1);

        size_t j=0;
        for (auto x : res)
        {
            REQUIRE (x == (i+j));
            j++;
        }

        i++;

        break;
    }

    // We dump some statistics.
    //launcher.getStatistics().dump(true);
}

template<typename PROC_UNIT, typename ...ARGS>
void VectorAsOutput_aux (PROC_UNIT pu, ARGS... args)
{
    // IMPORTANT -> max value allowed that avoids infinite loop in VectorAsOutput
    VectorAsOutput_aux_aux <VectorAsOutputUint8>  (pu, 224);
    VectorAsOutput_aux_aux <VectorAsOutputUint16> (pu, 16000);
    VectorAsOutput_aux_aux <VectorAsOutputUint32> (pu, 1*1000*1000);
}

TEST_CASE ("VectorAsOutput", "[Vector]" )
{
    VectorAsOutput_aux (ArchUpmem    ::DPU    { 1}, false);
    VectorAsOutput_aux (ArchMulticore::Thread {16});
}

//////////////////////////////////////////////////////////////////////////////
class CustomAllocator
{
public:
    using address_t = uint64_t;;

    static constexpr bool is_freeable = true;

    static address_t get (size_t sizeInBytes)  {  return (address_t) new char[sizeInBytes];  }

    static address_t write (void* src, size_t n)
    {
        auto a = get (n);
        memcpy ((void*)a, src, n);
        return a;
    };

    static address_t writeAt (address_t dest, void* data, size_t sizeInBytes)
    {
        memcpy ((void*)dest, data, sizeInBytes);
        return dest;
    }

    static address_t* read (address_t src, void* tgt, size_t n)
    {
        memcpy (tgt, (void*) src, n);
        return (address_t*) tgt;
    };

    static void free (address_t a)
    {
        char* ptr = (char*)a;
        delete[] ptr;
    }
};

template<typename ARCH>
struct VectorMulticoreFct
{
    using vector_t = bpl::core::vector<uint32_t,CustomAllocator,2,3,8>;

    auto operator() (const vector_t& v) // -> vector_t
    {
        vector_t result;
        for (uint32_t i=0; i<v.size(); i++)  {  result.push_back (2*v[i]);  }
        return result;
    }
};

TEST_CASE ("VectorMulticore", "[Vector]" )
{
    uint32_t imax = 10;

    using vector_t = typename VectorMulticoreFct<ArchMulticore>::vector_t;

    vector_t in;
    for (uint32_t i=1; i<=imax; i++)  { in.push_back (i); }

//in.flush();

    VectorMulticoreFct<ArchMulticore> fct;
    auto res = fct(in);

    REQUIRE (res.size() == in.size());

    for (size_t i=0; i<res.size(); i++)  { REQUIRE (res[i]==2*in[i]); }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorOfVectors", "[Vector]" )
{
    using type = typename VectorOfVectors<ArchMulticore>::type;

    std::vector<type> v;

    for ( [[maybe_unused]] auto& [stats,result] : Runner<>::run<VectorOfVectors> (v))
    {
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorFlush", "[Vector]" )
{
    for (size_t n : {1,2,3,5,8,13,21,34,55,89})
    {
        for (auto& [stats,result] : Runner<>::run<VectorFlush> (n))
        {
            for (auto& x : result)
            {
                REQUIRE (x.size() == n);

                for (size_t i=0; i<n; i++)
                {
                    REQUIRE (x[i] == i);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorSwap", "[Vector]" )
{
    auto test = [&] (size_t n)
    {
        for (auto& [stats,results] : Runner<>::run<VectorSwap> (n))
        {
            for (auto&& result : results)
            {
                REQUIRE (result.size() == 2*n);

                for (size_t i=0; i<result.size(); i++)
                {
                    REQUIRE (result[i] == result.size()-i);
                }
            }
        }
    };

    for (size_t n : {1,10,100,1000,10000})  { test(n);    }
    for (size_t n=1; n<=100; n++)           { test(n);    }
    for (size_t n=1; n<=14;  n++)           { test(1<<n); }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorAdd1", "[Vector]" )
{
    size_t nbranks = 20;
    size_t nbdpu   = nbranks*64;

    auto build = [] (size_t n, size_t k)
    {
        std::vector<uint32_t> v(n);  for (size_t i=0; i<v.size(); i++)  { v[i] = k*(i+1); } return v;
    };

    size_t nbitems = nbdpu*1000*1000;

    auto v1 = build (nbitems,1);
    auto v2 = build (nbitems,2);

    [[maybe_unused]] auto t0 = timestamp();

    Launcher<ArchUpmem> launcher { ArchUpmem::DPU(nbdpu), false};

    auto res = launcher.run<VectorAdd> (split(v1), split(v2));

    [[maybe_unused]] auto t1 = timestamp();

    size_t nbErrors1 = 0;
    size_t nbErrors2 = 0;

    auto results = res.retrieve();
	//   auto& results = res;

    for (auto& v : results)
    {
        nbErrors1 += v.size() == nbitems / launcher.getProcUnitNumber() ? 0 : 1;
    }

    [[maybe_unused]] auto t2 = timestamp();

#if 1
    size_t count = 0;
    for (auto& v : results)
    {
        for (size_t i=0; i<v.size(); i++)  {  nbErrors2 += v[i] == ++count * 3 ? 0 : 1;  }
    }

    REQUIRE (nbErrors1 == 0);
    REQUIRE (nbErrors2 == 0);
    REQUIRE (count     == nbitems);
#endif

    //printf ("time: %.3f  %.3f  #nbitems: %ld  #errors: [%ld,%ld] \n", (t1-t0)/1000.0, (t2-t1)/1000.0, nbitems, nbErrors1, nbErrors2 );
    launcher.getStatistics().dump();
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorMove1", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { 1_rank };

    for (auto res : launcher.run<VectorMove1> ())
    {
        REQUIRE (res.size()==2);
        REQUIRE (res[0]==1);
        REQUIRE (res[1]==2);
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorMove2", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { 1_rank };

    std::vector<uint32_t> v  { 1,2 };

    for (auto res : launcher.run<VectorMove2> (v))
    {
        REQUIRE (res.size()==2);
        REQUIRE (res[0]==1);
        REQUIRE (res[1]==2);
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorEmplaceBack", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { 1_rank };

    for (auto res : launcher.run<VectorEmplaceBack> ())
    {
        REQUIRE (res.size()==2);
        REQUIRE (res[0].a==1);   REQUIRE (res[0].x==3.14f);
        REQUIRE (res[1].a==5);   REQUIRE (res[1].x==2.71f);
    }
}
