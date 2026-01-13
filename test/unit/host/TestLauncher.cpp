////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <common.hpp>

using namespace bpl;

#include <tasks/Parrot1.hpp>
#include <tasks/Parrot2.hpp>
#include <tasks/Parrot3.hpp>
#include <tasks/Parrot4.hpp>
#include <tasks/Parrot5.hpp>
#include <tasks/Checksum1.hpp>
#include <tasks/Checksum2.hpp>
#include <tasks/Sum.hpp>
#include <tasks/SumGeneric.hpp>
#include <tasks/CountInstances.hpp>
#include <tasks/GetPuid.hpp>
#include <tasks/Array1.hpp>
#include <tasks/Array2.hpp>
#include <tasks/DoSomething.hpp>
#include <tasks/HelloWorld.hpp>
#include <tasks/Compare1.hpp>
#include <tasks/Compare2.hpp>
#include <tasks/ReturnArray.hpp>
#include <tasks/SplitRangeInt.hpp>
#include <tasks/TemplateTask.hpp>
#include <tasks/LauncherPool1.hpp>
#include <tasks/Max.hpp>
#include <tasks/Min.hpp>

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
};

////////////////////////////////////////////////////////////////////////////////
template<class ARCH, template<class> class TASK, typename... ARGS>
static auto TestParrot (size_t nbpu, std::tuple<ARGS...>&& answer)
{
    Launcher<ARCH> launcher;

    auto results = std::apply ([&] (auto &&... args)
    {
        return launcher.template run<TASK> (std::forward<ARGS>(args)...);
    }, answer);

    REQUIRE (results.size() == launcher.getProcUnitNumber());
    for (auto&& x : results)  {  REQUIRE (x==answer);  }
}

template<class ARCH>
static auto TestParrot_aux ()
{
    using namespace bpl;

    size_t nbpu = 2;

    TestParrot<ARCH,Parrot3> (nbpu, bpl::make_params<Parrot3<ARCH>> (
        MyData {333, 222, 2.71, 111},
        ARCH::make_pair (12,34),
        'A',
        ARCH::make_tuple ('B', 56, 3.141592),
        'C',
        ARCH::make_tuple (11, MyData {99, 88, 3.14159265, 77})
    ));

//     TestParrot<ARCH,Parrot2> (nbpu, std::make_tuple<std::string, std::vector<int>> ("abcdef", {1,1,2,3,5} ));
}

TEST_CASE ("Parrot", "[Launcher]" )
{
    //TestParrot_aux<ArchMulticore> ();
    TestParrot_aux<ArchUpmem> ();
}

////////////////////////////////////////////////////////////////////////////////
//TEST_CASE ("Launcher check properties", "[Launcher]" )
//{
//    REQUIRE (Launcher<ArchMulticore>().name() == "multicore");
//
//    REQUIRE (Launcher<ArchMulticore>(  ).getProcUnitNumber() ==  1);
//    REQUIRE (Launcher<ArchMulticore>( 2).getProcUnitNumber() ==  2);
//    REQUIRE (Launcher<ArchMulticore>(32).getProcUnitNumber() == 32);
//}

#if 1

////////////////////////////////////////////////////////////////////////////////
template<class ARCH, template<class> class TASK, typename CHECK>
static void TestChecksumNoSplit_aux (size_t nbProcessUnits, CHECK check)
{
    // We populate the buffer.
    NS(ARCH) array<int,1000> buffer;
    for (size_t i=0; i<buffer.size(); i++)  { buffer[i] = i; }

    size_t checkValue = buffer.size()*(buffer.size()-1) / 2;

    // We initialize our launcher.
    auto config = typename ARCH::DPU(nbProcessUnits);
    Launcher<ARCH> launcher (config);

    // We compute the intermediate results
    auto results = launcher.template run<TASK> (
        "some unit test",
        buffer,
        NS(ARCH) pair<size_t,size_t>(0,buffer.size())
    );

    // We check the results.
    check (results, launcher.getProcUnitNumber(), checkValue);
}

template<class ARCH>
static void TestChecksumNoSplit ()
{
    int indexes[] = {1,2,4,8,16};

    for (int i : indexes)
    {
        TestChecksumNoSplit_aux<ARCH,Checksum1> (i, [] (auto results, size_t nbpu, size_t checkValue)
        {
            REQUIRE (results.size() == nbpu);

            // Since we did not split the input parameters, we don't have any parallelization.
            // This means that all process units have computed the full checksum.
            for (auto x : results)   {  REQUIRE (x == checkValue);  }
        });
    }

    for (int i : indexes)
    {
        TestChecksumNoSplit_aux<ARCH,Checksum2> (i, [] (auto results, size_t nbpu, size_t checkValue)
        {
            static_assert (std::is_same<decltype(results),size_t>::value, "bad result type for Checksum2");

            // Checksum2 has a 'reduce' method, so the result is the aggregated one.
            // We have still no parallelization, so we have 'nbpu' times more than the real check value.
            REQUIRE (results == checkValue*nbpu);
        });
    }
}

TEST_CASE ("Launcher checksum no split (multicore)", "[Launcher]" )
{
    // We run the test on several architectures.

    // TestChecksumNoSplit<ArchMulticore> ();
    // TestChecksumNoSplit<ArchUpmem>     ();
}
#endif

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Sum", "[Launcher]" )
{
    Launcher<ArchUpmem> launcher (32_dpu);
    for (auto r : launcher.run<Sum> (1,3,5,8))  {  REQUIRE (r == 17);  }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SumGeneric", "[Launcher]" )
{
    using ARCH = ArchUpmem;

    Launcher<ARCH> launcher (ArchUpmem::Rank(4));
    //auto res = launcher.run<SumGeneric> ();
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("CountInstances", "[Launcher]" )
{
    Launcher<ArchUpmem> launcher (1_rank);

    auto res = launcher.run<CountInstances> (CountInstances<ArchUpmem>::Info {123});

    if (false)  {  CountInstances<ArchUpmem>::Info::dump();  }

    // We should have only one "true" construction (ie other than move constructor call)
    REQUIRE (CountInstances<ArchUpmem>::Info::counts(0) - CountInstances<ArchUpmem>::Info::counts(4) == 1);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Parrot3", "[Launcher]" )
{
    auto res = Launcher<ArchUpmem>(4_rank).run<Parrot3>
    (
        MyData {333, 222, 2.71, 111},
        ArchUpmem::make_pair (12,34),
        'A',
        ArchUpmem::make_tuple ('B', 56, 3.141592),
        'C',
        ArchUpmem::make_tuple (11, MyData {99, 88, 3.14159265, 77})
    );

    for (const auto& r : res)
    {
        REQUIRE (std::get<0>(r) == MyData {333, 222, 2.71, 111});
        REQUIRE (std::get<1>(r) == ArchUpmem::make_pair (12,34));
        REQUIRE (std::get<2>(r) == 'A');
        REQUIRE (std::get<3>(r) == ArchUpmem::make_tuple ('B', 56, 3.141592));
        REQUIRE (std::get<4>(r) == 'C');
        REQUIRE (std::get<5>(r) == ArchUpmem::make_tuple (11, MyData {99, 88, 3.14159265, 77}));
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Parrot4", "[Launcher]" )
{
    auto res = Launcher<ArchUpmem>(1_rank).run<Parrot4>
    (
        ArchUpmem::array<SomeData,8> {
            SomeData { 1, 2, 1.12345, 'A'},
            SomeData { 3, 4, 2.12345, 'B'},
            SomeData { 5, 6, 3.12345, 'C'},
            SomeData { 7, 8, 4.12345, 'D'},
            SomeData { 9,10, 5.12345, 'E'},
            SomeData {11,12, 6.12345, 'F'},
            SomeData {13,14, 7.12345, 'G'},
            SomeData {15,16, 8.12345, 'H'}
        },
        ArchUpmem::array<char,8> { 'm','a','i','s','o','n'},
        4567,
        3.141592,
        '*'
   );
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Array1", "[Launcher]" )
{
    Array1<ArchUpmem>::array_t arr;
    for (size_t i=0; i<arr.size(); i++)  { arr[i] = i; }

    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(1));

    uint64_t truth = uint64_t(arr.size()) * (arr.size()-1)/2 * launcher.getProcUnitNumber();

    REQUIRE (launcher.run<Array1>  (arr, 1) == truth*1);
    REQUIRE (launcher.run<Array1>  (arr, 2) == truth*2);
    REQUIRE (launcher.run<Array1>  (arr, 4) == truth*4);
}

TEST_CASE ("Array2upmem", "[Launcher]" )
{
    Array2<ArchUpmem>::array_t arr;
    for (size_t i=0; i<arr.size(); i++)  { arr[i] = i; }

    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(1));

    uint64_t truth = uint64_t(arr.size()) * (arr.size()-1)/2 * launcher.getProcUnitNumber();

    REQUIRE (launcher.run<Array2>  (arr, 1) == truth*1);
    REQUIRE (launcher.run<Array2>  (arr, 2) == truth*2);
    REQUIRE (launcher.run<Array2>  (arr, 4) == truth*4);
}

TEST_CASE ("Array2multicore", "[Launcher]" )
{
    Array2<ArchMulticore>::array_t arr;
    for (size_t i=0; i<arr.size(); i++)  { arr[i] = i; }

    Launcher<ArchMulticore> launcher (32_thread);

    uint64_t truth = uint64_t(arr.size()) * (arr.size()-1)/2 * launcher.getProcUnitNumber();

    REQUIRE (launcher.run<Array2>  (arr, 1) == truth*1);
    REQUIRE (launcher.run<Array2>  (arr, 2) == truth*2);
    REQUIRE (launcher.run<Array2>  (arr, 4) == truth*4);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Mix runs", "[Launcher]" )
{
    Array1<ArchUpmem>::array_t arr;
    for (size_t i=0; i<arr.size(); i++)  { arr[i] = i & 255; }

    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(1));

    if (false)
    {
        printf ("[Array1] sum: %ld\n", launcher.run<Array1>  (arr, 1));
        printf ("[GetPuid]     %ld\n", launcher.run<GetPuid> ());
        printf ("[Array1] sum: %ld\n", launcher.run<Array1>  (arr, 1));
        printf ("[Array1] sum: %ld\n", launcher.run<Array1>  (arr, 2));
        printf ("[GetPuid]     %ld\n", launcher.run<GetPuid> ());
        printf ("[Array1] sum: %ld\n", launcher.run<Array1>  (arr, 1));
        printf ("[Array1] sum: %ld\n", launcher.run<Array1>  (arr, 2));
        printf ("[Array1] sum: %ld\n", launcher.run<Array1>  (arr, 4));
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("GetPuid", "[Launcher]" )
{
    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(4));

    auto res = launcher.run<GetPuid> ();

    REQUIRE (res == launcher.getProcUnitNumber()*(launcher.getProcUnitNumber()-1)/2);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("DoSomething", "[Launcher]" )
{
    std::array<char,8> data {'h','i',' ','w','o','r','l','d'};

    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(4));

    launcher.run<DoSomething> (data, 123);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("HelloWorld", "[Launcher]" )
{
    std::array<char,16> data {'h','e','l','l', 'o', ' ', 'w','o','r','l','d', ' ', '!', 0};

    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(4));

    for (auto res : launcher.run<HelloWorld> (data))
    {
        REQUIRE (res==1181);
    }
}

//////////////////////////////////////////////////////////////////////////////
//TEST_CASE ("Compare1", "[Bank]" )
//{
//    using resources_t = bpl::ArchMulticoreResources;
//
//    const static int NBREF  = Compare1<resources_t>::NBREF;
//    const static int NBQRY  = Compare1<resources_t>::NBQRY;
//    const static int SEQLEN = Compare1<resources_t>::SEQLEN;
//
//    RandomSequenceGenerator<SEQLEN> generator;
//    BankChunk <resources_t,SEQLEN,NBREF> bankRef (generator);
//    BankChunk <resources_t,SEQLEN,NBQRY> bankqry (generator);
//
//    std::pair<int,int> range = {0, bankqry.size()};
//
//    float first = 0.0;
//
//    std::vector<int> nbvalues;
//    nbvalues.push_back(1);
//    for (int nb=2; nb<=2048; nb+=2)  { nbvalues.push_back(nb); }
//
////    for (int nb : {1,2,4,8,16,32,64,128,256,512,1024})
////    for (int nbdpu : {1024,2048})
//    //for (int nb : {1,2,4,8,16})
////    for (int nb=2; nb<=2048; nb+=2)
////    for (int nb : nbvalues)
//    for (int nb : {1,2,4,8,16,32,64,128,256,512,1024, 2048})
////    for (int nb=64; nb<=64*40; nb+=64)
//    {
//        auto config = ArchUpmem::DPU(nb);
////        auto config = ArchUpmem::Rank(nb);
//
//        Launcher<ArchUpmem> launcher (config);
//
//        for (size_t t=0; t<=100; t+=1000)
//        {
//            auto result = launcher.run<Compare1> (
//                bankRef,
//                bankqry,
//                split<ArchUpmem::Tasklet,CONT>(range),
//                t
//            );
//
////            printf ("RESULT: threshold=%3d  result=%d\n", t, result);
////            printf ("%3d %d\n", t, result);
//        }
//
//        auto& stats = launcher.getStatistics();
//
////        stats.dump(true);
//
//        const auto& timings = stats.getTimings();
//        auto lookup = timings.find ("run/launch");
//        if (lookup != timings.end())
//        {
//            if (first==0)  { first=lookup->second; }
//
//            printf ("%4d  %8.3f  %8.3f  %.3f\n",
//                nb, lookup->second, first/lookup->second,
//                first/lookup->second / float(nb)
//            );
//        }
//    }
//}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("ReturnArray", "[Launcher]" )
{
    //using config_t = config::multicore;
    using config_t = config::upmem;

    using arch_t   = typename config_t::arch_t;

    Launcher<arch_t> launcher {typename config_t::resource_t {4}};

    ReturnArray<arch_t>::type_t    K {100};
    ReturnArray<arch_t>::myarray_t data;

    for (size_t i=0; i<data.size(); i++)  { data[i] = 10+i; }

    auto result = launcher.run<ReturnArray> (data, K);

    size_t i=0;
    for (auto res : result)
    {
        REQUIRE (res.size() == data.size());

        for (size_t j=0; j<res.size(); j++)
        {
            REQUIRE (res[j] == data[j]+K+i);
        }

        i++;
    }
}

//////////////////////////////////////////////////////////////////////////////

template<typename ARCH, typename...ARGS>
static constexpr bool concepts_aux (ARGS&&... args)
{
    return std::conjunction_v <matching_splitter<ARCH,ARGS>... >;
}

TEST_CASE ("Concepts", "[Launcher]" )
{
    auto r { Range(1,10) };

    REQUIRE (false == concepts_aux<ArchMulticore> (split<ArchUpmem::Tasklet>(r)));
    REQUIRE (true  == concepts_aux<ArchUpmem>     (split<ArchUpmem::Tasklet>(r)));
    REQUIRE (true  == concepts_aux<ArchUpmem>     (split<ArchUpmem::Tasklet>(r),    split<ArchUpmem::Tasklet>(r)));
    REQUIRE (true  == concepts_aux<ArchMulticore> (split<ArchMulticore::Thread>(r), split<ArchMulticore::Thread>(r)));
    REQUIRE (false == concepts_aux<ArchUpmem>     (split<ArchUpmem::Tasklet>(r),    split<ArchMulticore::Thread>(r)));
    REQUIRE (false == concepts_aux<ArchMulticore> (split<ArchUpmem::Tasklet>(r),    split<ArchUpmem::Tasklet>(r)));
    REQUIRE (false == concepts_aux<ArchMulticore> (split<ArchUpmem::Tasklet>(r),    split<ArchMulticore::Thread>(r)));

    using T1 = decltype(r);
    using T2 = decltype(split<ArchUpmem::Tasklet>(r));

    REQUIRE (is_splitter_v <T1> == false);
    REQUIRE (is_splitter_v <T2> == true);

    REQUIRE (matching_splitter <ArchUpmem,    T1>::value == true);
    REQUIRE (matching_splitter <ArchMulticore,T1>::value == true);

    REQUIRE (matching_splitter <ArchUpmem,    T2>::value == true);
    REQUIRE (matching_splitter <ArchMulticore,T2>::value == false);
}

//////////////////////////////////////////////////////////////////////////////
template<typename LAUNCHER>
void template_task_aux (LAUNCHER launcher)
{
    double value {145};
    for (auto x : launcher.template run<TemplateTask,decltype(value)> (value))   {  REQUIRE (x==value);  }
}

TEST_CASE ("template task", "[Launcher]" )
{
    template_task_aux (Launcher<ArchMulticore> {4_thread});
    template_task_aux (Launcher<ArchUpmem>     {4_dpu});
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("LauncherPool0", "[Launcher]" )
{
    std::array<char,16> data {'h','e','l','l', 'o', ' ', 'w','o','r','l','d', ' ', '!', 0};

    LauncherPool<ArchUpmem> pool (25, 1_rank);

    size_t imax = 20 * pool.getNbComponents();

    std::atomic<uint32_t> nbOk = 0;

    auto cbk = [&nbOk] (auto&& launcher, auto&& results)
    {
        for (auto res : results)
        {
            if (res==1181)   {  nbOk++;  }
        }
    };

    // WARNING... we use a temporary object 'cbk' for calling 'submit'
    // => using directly the lambda as argument may lead to to freeze on 'wait' (KO with g++ but OK with clang++)
    for (size_t i=0; i<imax; i++)
    {
        pool.submit<HelloWorld> (cbk, std::ref(data));
    }

   // We need to wait the end of all workers.
    pool.wait();

    // We look whether we got all the results. NOTE: since some DPU may not work (HW issue),
    // we are not sure to have a 1 ratio here, so we test against a reasonable value like 0.99
    REQUIRE (nbOk.load() > 0.99 * (NR_TASKLETS * 64 * imax) );
}

//////////////////////////////////////////////////////////////////////////////
auto LauncherPool1_aux (LauncherPool<ArchUpmem>& pool)
{
    uint16_t nbLaunchers = pool.size();
    uint16_t nbRuns = 2;
    uint16_t totalNbRuns = nbRuns*nbLaunchers;

    std::atomic<uint16_t> totalNbOk = 0;

    auto callback = [&totalNbOk] (auto&& launcher, auto&& results)
     {
        uint16_t nbOk = 0;
        for (auto res : results)  {  nbOk += res; }
        totalNbOk += nbOk; // take care of concurrent access since we are in a specific thread here.
     };

    for (uint16_t n=0; n<totalNbRuns; n++)
    {
        pool.submit<LauncherPool1> (callback, n);
    }

    // We wait for the end of all tasks processed by the pool.
    pool.wait();

    uint16_t res = (totalNbRuns*(totalNbRuns-1)/2) * 1 * NR_TASKLETS;
    REQUIRE (totalNbOk.load() == res);
}

TEST_CASE ("LauncherPool1", "[Launcher]" )
{
    LauncherPool<ArchUpmem> pool (2, 1_dpu);
    LauncherPool1_aux (pool);
}

//////////////////////////////////////////////////////////////////////////////
template<bool B>
struct LauncherPoolTest
{
    LauncherPoolTest ()                          { stats()[0]++; }
    LauncherPoolTest (LauncherPoolTest const& x) { stats()[1]++; }
    LauncherPoolTest (LauncherPoolTest     && x) { stats()[2]++; }
    ~LauncherPoolTest ()                         { stats()[3]++; }
    static auto&  stats()  {  static std::array<size_t,4> arr = {};  return arr;  }
};

template<class ARCH>  struct LauncherPool2
{
    auto operator() (LauncherPoolTest<true> const& x, LauncherPoolTest<false> const& y) const  {  return 0;  }
};

TEST_CASE ("LauncherPool2", "[Launcher]" )
{
    // inside block => make sure the resources will be released before checking the instances number
    {
        LauncherPool<ArchMulticore> pool (1, 1_thread);

        LauncherPoolTest<true> obj;

        pool.submit<LauncherPool2> (
            [] (auto&& launcher, auto&& results) {},
            std::ref(obj),               // We use std::ref here since we know that 'obj' will live long enough.
            LauncherPoolTest<false>{}    // A temporary object.
        );

        pool.wait();
    }

    // For the left reference:   1 default
    REQUIRE (LauncherPoolTest<true> ::stats() == std::array<size_t,4> {1,0,0,1});

    // For the temporary object: 1 default, 1 copy and 2 moves
    REQUIRE (LauncherPoolTest<false>::stats() == std::array<size_t,4> {1,1,2,4});
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Max1", "[MaxMin]" )
{
    Launcher<ArchUpmem> launcher (64_dpu);
    for (auto r : launcher.run<Max> (17, 42))  {  REQUIRE (r == 42);  }
    for (auto r : launcher.run<Max> (36, 12))  {  REQUIRE (r == 36);  }
}

TEST_CASE ("Max2", "[MaxMin]" )
{
    Launcher<ArchMulticore> launcher (32_thread);
    for (auto r : launcher.run<Max> (17, 42))  {  REQUIRE (r == 42);  }
    for (auto r : launcher.run<Max> (36, 12))  {  REQUIRE (r == 36);  }
}

TEST_CASE ("Min1", "[MaxMin]" )
{
    Launcher<ArchUpmem> launcher (64_dpu);
    for (auto r : launcher.run<Min> (17, 42))  {  REQUIRE (r == 17);  }
    for (auto r : launcher.run<Min> (36, 12))  {  REQUIRE (r == 12);  }
}

TEST_CASE ("Min2", "[MaxMin]" )
{
    Launcher<ArchMulticore> launcher (32_thread);
    for (auto r : launcher.run<Min> (17, 42))  {  REQUIRE (r == 17);  }
    for (auto r : launcher.run<Min> (36, 12))  {  REQUIRE (r == 12);  }
}
