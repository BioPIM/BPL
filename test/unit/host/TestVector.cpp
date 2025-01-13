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

using namespace bpl;

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
#include <tasks/VectorCache.hpp>
#include <tasks/VectorConfig.hpp>
#include <tasks/VectorMany.hpp>
#include <tasks/VectorMany1.hpp>
#include <tasks/VectorMany2.hpp>
#include <tasks/VectorMany2b.hpp>
#include <tasks/VectorMany3.hpp>
#include <tasks/VectorMany4.hpp>
#include <tasks/VectorViewRandomAccess1.hpp>
#include <tasks/VectorViewRandomAccess2.hpp>
#include <tasks/VectorAddAndIterate.hpp>
#include <tasks/VectorSizeof12.hpp>
#include <tasks/VectorRandomAccessGlobal1.hpp>
#include <tasks/VectorRandomAccessGlobal2.hpp>
#include <tasks/VectorEmpty1.hpp>
#include <tasks/VectorEmpty2.hpp>

#include <Runner.hpp>

#include <chrono>
#include <fmt/core.h>
#include <fmt/ranges.h>

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
    Launcher<ArchUpmem> launcher (ArchUpmem::DPU{1});

    uint32_t nbitems = 1300*4*8 + 20;

    for (size_t i=0; i<10; i++)
    {
        auto result = launcher.run<Vector1> (nbitems);

        for (auto res : result)
        {
            REQUIRE (res == nbitems);
        }
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

    std::vector<uint32_t> v (nbitems);
    std::generate (v.begin(), v.end(), [i=0]() mutable  { i+=3; return i; });
    uint64_t truth = std::accumulate (v.begin(), v.end(), uint64_t(0));

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
//    VectorInputOutput_aux <VectorInputOutput2> (std::vector<size_t> {1,10,100,1000});
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorInputOutputUpmem", "[Vector]" )
{
    std::vector<uint32_t> v;  for (size_t i=1; i<=10; i++)  { v.push_back(i); }

    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1), false /* traces yes/no */);

//    auto result = launcher.run<VectorInputOutput2> (v);
//
//    for (auto& res : result)
//    {
//        for (size_t i=0; i<v.size(); i++)
//        {
//            //printf ("RES: ==> %d %d \n", 2*v[i], res[i]);
//            REQUIRE (2*v[i] == res[i]);
//        }
//    }
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
    template<int N>  using address_array_t = address_t [N];

    static constexpr bool is_freeable = true;

    static address_t get (size_t sizeInBytes)  {  return (address_t) new char[sizeInBytes];  }

    static address_t write (void* src, size_t n)
    {
        auto a = get (n);
        memcpy ((void*)a, src, n);
        return a;
    };

    static address_t writeAt (void* dest, void* data, size_t sizeInBytes)
    {
        memcpy ((void*)dest, data, sizeInBytes);
        return (address_t)dest;
    }

    static auto writeAtomic (address_t tgt, address_t & src)
    {
        writeAt ((void*)tgt, &src, sizeof(address_t));
    }

    static address_t* read (void* src, void* tgt, size_t n)
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
    struct MutexNull { auto lock() {} auto unlock(); };

    using vector_t = bpl::vector<uint32_t,CustomAllocator,MutexNull,2,1,3,8>;

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
//    using type = typename VectorOfVectors<ArchMulticore>::type;
//
//    std::vector<type> v;
//
//    for ( [[maybe_unused]] auto& [stats,result] : Runner<>::run<VectorOfVectors> (v))
//    {
//    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorFlush", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { ArchUpmem::DPU(1), false};

    size_t n = 1;

    auto result = launcher.run<VectorFlush> (n);
    for (auto& x : result)
    {
        REQUIRE (x.size() == n);

        for (size_t i=0; i<n; i++)
        {
            REQUIRE (x[i] == i);
        }
    }

return;

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
    Launcher<ArchUpmem> launcher { 1_rank};

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
    Launcher<ArchUpmem> launcher { 1_rank};

    for (auto res : launcher.run<VectorEmplaceBack> ())
    {
        REQUIRE (res.size()==2);
        REQUIRE (res[0].a==1);   REQUIRE (res[0].x==3.14f);
        REQUIRE (res[1].a==5);   REQUIRE (res[1].x==2.71f);
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorCache", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { 1_dpu};

    for ([[maybe_unused]] auto res : launcher.run<VectorCache> ( std::vector<uint64_t>{1,2,3}, std::vector<uint8_t>{4,5,6,7}))
    {
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorConfig", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { 1_dpu};

    for ([[maybe_unused]] auto res : launcher.run<VectorConfig> ( std::vector<uint64_t>{1,2,3}, std::vector<uint8_t>{4,5,6,7}))
    {
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorMany", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { 1_dpu};

    auto result = launcher.run<VectorMany> (
        std::vector<uint32_t>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        std::vector<uint32_t>{10,11,12,13,14,15,16,17,18,19},
        std::vector<uint32_t>{20,21,22,23,24,25,26,27,28,29},
        std::vector<uint32_t>{30,31,32,33,34,35,36,37,38,39},
        std::vector<uint32_t>{40,41,42,43,44,45,46,47,48,49},
        std::vector<uint32_t>{50,51,52,53,54,55,56,57,58,59},
        std::vector<uint32_t>{60,61,62,63,64,65,66,67,68,69}
    );

    for ([[maybe_unused]] auto res : result)
    {
        REQUIRE (res.size()==70);

        uint32_t check = 0;
        for (uint32_t x : res)  {  REQUIRE (x == check++);  }
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorMany1", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { 1_dpu};

    auto result = launcher.run<VectorMany> (
        std::vector<uint32_t>{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        std::vector<uint32_t>{10,11,12,13,14,15,16,17,18,19},
        std::vector<uint32_t>{20,21,22,23,24,25,26,27,28,29},
        std::vector<uint32_t>{30,31,32,33,34,35,36,37,38,39},
        std::vector<uint32_t>{40,41,42,43,44,45,46,47,48,49},
        std::vector<uint32_t>{50,51,52,53,54,55,56,57,58,59},
        std::vector<uint32_t>{60,61,62,63,64,65,66,67,68,69}
    );

    for ([[maybe_unused]] auto res : result)
    {
        REQUIRE (res.size()==70);

        uint32_t check = 0;
        for (uint32_t x : res)  {  REQUIRE (x == check++);  }
    }
}

//////////////////////////////////////////////////////////////////////////////
template<template<typename> class TASK>
auto VectorMany2_aux ()
{
    Launcher<ArchUpmem> launcher { 1_dpu, false};

    typename TASK<ArchUpmem>::type t;

    const size_t nbfields = get_nb_fields (t);

    size_t nbitems = 51;
    size_t n       = 1;

    auto fill = [&] (auto&& obj)
    {
        bpl::class_fields_iterate (obj, [&] (auto&& field)
        {
            for (size_t i=0; i<nbitems; i++)  {  field.push_back (n++); }
        });
    };

    fill (t);

    REQUIRE (n==nbfields*nbitems+1);

    for (auto res : launcher.run<TASK> (t))
    {
        REQUIRE (res.size() == nbfields*nbitems);

        size_t n=1;
        for (uint32_t x : res)  {  REQUIRE (x == n++);  }
    }

    //launcher.getStatistics().dump (true);
}

TEST_CASE ("VectorMany2", "[Vector]" )
{
    VectorMany2_aux <VectorMany2 > ();
    VectorMany2_aux <VectorMany2b> ();
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorMany3", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { 1_dpu, false};

    VectorMany3<ArchUpmem>::type t;

    size_t nbitems = 30*1000;  // we will use nbitems*10(vectors)*16(tasklets)*sizeof(uint32_t) bytes
    size_t n0      = 1;

    size_t n=n0;
    for (size_t i=0; i<nbitems; i++)  {  t.a0.push_back (n++); }
    for (size_t i=0; i<nbitems; i++)  {  t.a1.push_back (n++); }
    for (size_t i=0; i<nbitems; i++)  {  t.a2.push_back (n++); }
    for (size_t i=0; i<nbitems; i++)  {  t.a3.push_back (n++); }
    for (size_t i=0; i<nbitems; i++)  {  t.a4.push_back (n++); }
    for (size_t i=0; i<nbitems; i++)  {  t.a5.push_back (n++); }
    for (size_t i=0; i<nbitems; i++)  {  t.a6.push_back (n++); }
    for (size_t i=0; i<nbitems; i++)  {  t.a7.push_back (n++); }
    for (size_t i=0; i<nbitems; i++)  {  t.a8.push_back (n++); }
    for (size_t i=0; i<nbitems; i++)  {  t.a9.push_back (n++); }

    for (auto res : launcher.run<VectorMany3> (t))
    {
        REQUIRE (res.size() == 10*nbitems);

        size_t m=n0;
        for (uint32_t x : res)
        {
            REQUIRE (x == m++);
        }
    }

    //launcher.getStatistics().dump (true);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorMany4", "[Vector]" )
{
    Launcher<ArchUpmem> launcher { 178_dpu, false };

    VectorMany4<ArchUpmem>::type8  t8;
    VectorMany4<ArchUpmem>::type16 t16;
    VectorMany4<ArchUpmem>::type32 t32;
    VectorMany4<ArchUpmem>::type64 t64;

    size_t nbitems = 20;
    size_t n=0;

    auto fill = [&] (auto&& obj)
    {
        bpl::class_fields_iterate (obj, [&] (auto&& field)
        {
            for (size_t i=0; i<nbitems; i++)  {  field.push_back (n++); }
        });
    };

    fill (t8);
    fill (t16);
    fill (t32);
    fill (t64);

    for ([[maybe_unused]] auto res : launcher.run<VectorMany4> (t8,t16,t32,t64))
    {
        REQUIRE (res.size() == 4*3*nbitems);

        n=0;
        for (auto x : res)
        {
            REQUIRE (x == n);
            n++;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
template<template<typename> class TASK>
auto VectorViewRandomAccess_aux ()
{
    Launcher<ArchUpmem> launcher { 111_dpu, false};

    size_t nbitems = 1000*1000;

    uint64_t truth = 0;
    std::vector<uint32_t> v;  for (size_t i=1; i<=nbitems; i++)  { v.push_back(i);  truth += v.back(); }

    for (auto res : launcher.run<TASK> (v))  {  REQUIRE (res == truth);  }
}

TEST_CASE ("VectorViewRandomAccess1", "[Vector]" )
{
    VectorViewRandomAccess_aux<VectorViewRandomAccess1>();
}

TEST_CASE ("VectorViewRandomAccess2", "[Vector]" )
{
    VectorViewRandomAccess_aux<VectorViewRandomAccess2>();
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorAddAndIterate", "[Vector]")
{
    std::vector<uint32_t> timings;

    Launcher<ArchUpmem> launcher { 1_dpu, false };

    for (size_t i=1; i<=13; i++)
    {
        size_t nbitems = 1<<i;

        //fmt::println ("nbitems: {:6}", nbitems);

         auto t0 = std::chrono::high_resolution_clock::now();

         auto results = launcher.run<VectorAddAndIterate> (nbitems);

         auto t1 = std::chrono::high_resolution_clock::now();

         timings.push_back ( std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count() );

         for (auto res : results)  {  REQUIRE (nbitems == res);  }
     }

    // ratios of timings t(n+1)/t(n) should tend to 4 since the test is in O(n^2)
    //for (size_t i=0; i+1<timings.size(); i++)  {  printf ("%2ld  %6.4f \n", i, (double)timings[i+1]/timings[i]);  }
}

//////////////////////////////////////////////////////////////////////////////
auto VectorSizeof12_aux (size_t nbdpu, uint16_t nbitems)
{
    Launcher<ArchUpmem> launcher { ArchUpmem::DPU(nbdpu), false };

    using Seed = VectorSizeof12<ArchUpmem>::Seed;

    uint64_t truth = uint64_t(nbitems)*(nbitems+1)/2;

    std::vector<Seed> seeds;
    for (uint16_t i=1; i<=nbitems; i++)  {  seeds.emplace_back (Seed { i, {i, i } });  }

    for (auto [x,y,z] : launcher.run<VectorSizeof12> (seeds))
    {
        REQUIRE (x == 1*truth);
        REQUIRE (y == 2*truth);
        REQUIRE (z == 3*truth);
    }
}

TEST_CASE ("VectorSizeof12", "[Vector]")
{
    for (size_t nbdpu : {1,4,16,64,256})
    {
        for (uint16_t n : {1,10,100,1000,10000,20000,40000,60000})  {  VectorSizeof12_aux (nbdpu, n);  }

        for (uint16_t n=1; n<=20; n++)  {  VectorSizeof12_aux (nbdpu, n);  }

        for (uint16_t n=1; n<=15; n++)  {  VectorSizeof12_aux (nbdpu, 1<<n);  }
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorRandomAccessGlobal1", "[Vector]")
{
    Launcher<ArchUpmem> launcher { 1_dpu, false };

    using type = VectorRandomAccessGlobal1<ArchUpmem>::type;

    size_t nbitems = 50*1000;

    type v (nbitems);
    std::iota (std::begin(v), std::end(v), 1);

    uint64_t truth = std::accumulate (std::begin(v), std::end(v), uint64_t{0});

    for (auto res : launcher.run<VectorRandomAccessGlobal1> (v))
    {
        REQUIRE (res == truth);
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorRandomAccessGlobal2", "[Vector]")
{
    Launcher<ArchUpmem> launcher { 1_dpu, false };

    using type = mydata<ArchUpmem>;

    size_t nbitems = 50*1000;
    uint64_t truth=0;
    type v;
    for (size_t i=1; i<=nbitems;  i++)  { v.data.push_back(i); truth += v.data.back(); }

    for (auto res : launcher.run<VectorRandomAccessGlobal2> (v))
    {
        REQUIRE (res == truth);
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorEmpty1", "[Vector]")
{
    Launcher<ArchUpmem> launcher { 1_dpu, false };

    std::vector<int> v ;

    for (auto res : launcher.run<VectorEmpty1> (v))
    {
        REQUIRE (res == v.size());
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("VectorEmpty2", "[Vector]")
{
    Launcher<ArchUpmem> launcher { 1_dpu, false };

    DataVectorEmpty<ArchUpmem> data = { {1}, {}, {2} };

    for (auto res : launcher.run<VectorEmpty1> (data))
    {
    }
}
