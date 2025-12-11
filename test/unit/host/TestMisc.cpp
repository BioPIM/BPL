////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <common.hpp>

#include <bpl/utils/RangeInt.hpp>
#include <bpl/utils/tag.hpp>

using namespace bpl;

#include <tasks/IntegerType.hpp>
#include <tasks/Once1.hpp>
#include <tasks/Global0.hpp>
#include <tasks/Global1.hpp>
#include <tasks/Global2.hpp>
#include <tasks/Global3.hpp>
#include <tasks/Global4.hpp>
#include <tasks/Global5.hpp>
#include <tasks/Global6.hpp>
#include <tasks/Struct1.hpp>
#include <tasks/Struct2.hpp>
#include <tasks/Struct3.hpp>
#include <tasks/GlobalOnce1.hpp>
#include <tasks/GlobalOnce2.hpp>
#include <tasks/GlobalOnce3.hpp>
#include <tasks/GlobalOnce4.hpp>
#include <tasks/OncePadding1.hpp>
#include <tasks/ResultsToVector.hpp>
#include <tasks/VectorSplitDpu.hpp>
#include <tasks/TemplateSpecialization.hpp>
#include <tasks/SyracuseReduce.hpp>
#include <tasks/Syracuse.hpp>

#include <iostream>
#include <list>
#include <ranges>
#include <algorithm>

#include <fmt/core.h>
#include <fmt/ranges.h>

////////////////////////////////////////////////////////////////////////////////
TEST_CASE ("IntegerType", "[Misc]" )
{
    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1));

    size_t nbItems = 225;  // PB WITH uint8_t

    auto result = launcher.run<IntegerType> (nbItems);
}

//////////////////////////////////////////////////////////////////////////////
void RangeInt_aux (size_t i0, size_t i1, size_t nbSplits)
{
    auto range = RangeInt (i0,i1);

    size_t checkVal = i0;
    size_t checkNb  = 0;

    for (size_t i=0; i<nbSplits; i++)
    {
        auto subrange = SplitOperator<RangeInt>::split (range, i, nbSplits);

        for (auto k : subrange)
        {
            REQUIRE (k == checkVal);
            checkNb++;
            checkVal++;
        }
    }

    REQUIRE ((i1-i0) == checkNb);
}

TEST_CASE ("RangeInt", "[Misc]" )
{
    srand(0);
    for (size_t i=1; i<=100; i++)
    {
        auto i0 = 0  + rand() % (1<<10);
        auto i1 = i0 + rand() % (1<<10);
        auto sp = 1 + (rand() % (1<<4));

        RangeInt_aux (i0, i1, sp);
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SplitPair1", "[Misc]" )
{
    using T = std::pair<uint64_t,uint64_t>;

    T range {0,1000};

    size_t nbSplits = 4;

    REQUIRE (SplitOperator<T>::split (range, 0, nbSplits) == T {  0, 250});
    REQUIRE (SplitOperator<T>::split (range, 1, nbSplits) == T {250, 500});
    REQUIRE (SplitOperator<T>::split (range, 2, nbSplits) == T {500, 750});
    REQUIRE (SplitOperator<T>::split (range, 3, nbSplits) == T {750,1000});
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SplitPair2", "[Misc]" )
{
    using T = std::pair<uint64_t,uint64_t>;

    size_t nbSplits = 64;

    for (size_t i=1; i<=2*nbSplits; i++)
    {
        T range {1,i};

        uint64_t checksum = 0;

        for (size_t j=0; j<nbSplits; j++)
        {
            auto subrange = SplitOperator<T>::split (range, j, nbSplits);

            for (size_t x=subrange.first; x<subrange.second; x++)  { checksum += x; }
        }
        REQUIRE (checksum == i*(i-1)/2);
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global", "[Misc]" )
{
    struct A
    {
        A()            { mask() |= 1; }
        A(const A&  )  { mask() |= 2; }
        A(const A&& )  { mask() |= 4; }
        static uint8_t& mask()  { static uint8_t value=0;  return value; }
    };

//    auto task = []<typename...ARGS> (ARGS&&... args)  {};
//
//    auto run = []<typename FCT, typename...ARGS> (FCT fct, ARGS&&... args) -> unsigned long long
//    {
//        constexpr unsigned long long mask = pack_create_mask_v<is_global,ARGS...>;
//
//        using partition_t = pack_filter_by_mask_partition_t <mask, ARGS...>;
//
//        fct (args...);
//        return mask;
//    };
//
//    [[maybe_unused]] auto a = global (A{});  // default + move constructors should be called
//    REQUIRE (A::mask() == 5);
//
//    auto g = global<ArchUpmem::Rank> (3);
//
//    static_assert (is_global_v<decltype(g)> == true);
//    static_assert (is_global_v<decltype(3)> == false);
//
//    static_assert (pack_create_mask_v <is_global, decltype(g), decltype(3)> == 0b01);
//    static_assert (pack_create_mask_v <is_global, decltype(3), decltype(g)> == 0b10);
//    static_assert (pack_create_mask_v <is_global, char, decltype(3), decltype(g), float, decltype(g)> == 0b10100);
//
//    static_assert (0b0101 == run (task,    global(3), 4, global(5)  ));
//    static_assert (0b1010 == run (task, 2, global(3), 4, global(5),6));
}

//////////////////////////////////////////////////////////////////////////////
struct dosomething
{
    auto operator() (
        once  <uint8_t>  a,
        global<uint16_t> b,
        int c,
        once<global<uint32_t>> d,
        global<once<uint64_t>> e
    )
    {  return 0;  }
};

TEST_CASE ("Tag1", "[tag]" )
{
    using namespace bpl;

    using params = task_params_t<dosomething>;

    REQUIRE (hastag_v <once,  std::tuple_element_t<0,params>> == true);

    REQUIRE (hastag_v <once,  std::tuple_element_t<0,params>> == true);
    REQUIRE (hastag_v <once,  std::tuple_element_t<1,params>> == false);
    REQUIRE (hastag_v <once,  std::tuple_element_t<2,params>> == false);

    REQUIRE (hastag_v <global, std::tuple_element_t<0,params>> == false);
    REQUIRE (hastag_v <global, std::tuple_element_t<1,params>> == true);
    REQUIRE (hastag_v <global, std::tuple_element_t<2,params>> == false);

    REQUIRE (hastag_v <global,std::tuple_element_t<3,params>> == true);
    REQUIRE (hastag_v <once,  std::tuple_element_t<3,params>> == true);

    REQUIRE (hastag_v <global,std::tuple_element_t<4,params>> == true);
    REQUIRE (hastag_v <once,  std::tuple_element_t<4,params>> == true);

    static_assert (std::is_same_v<first_non_tag<std::tuple_element_t<0,params>>::type,uint8_t>);
    static_assert (std::is_same_v<first_non_tag<std::tuple_element_t<1,params>>::type,uint16_t>);
    static_assert (std::is_same_v<first_non_tag<std::tuple_element_t<3,params>>::type,uint32_t>);
    static_assert (std::is_same_v<first_non_tag<std::tuple_element_t<4,params>>::type,uint64_t>);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Once1", "[once]" )
{
    Launcher<ArchUpmem> launcher (1_dpu);

    uint64_t truth = 0;
    size_t N = 1*1000*1000;

    std::vector<uint32_t> vec(N);  for (size_t i=0; i<N; i++)  {  truth += vec[i] = i+1; }

    for (size_t n=1; n<=10; n++)
    {
        // We check that we got the correct result.
        for (auto result : launcher.run<Once1> (vec,123))  {  REQUIRE (result==truth);  }

        // We check that we broadcast only once the vector
        size_t bufsize = launcher.getStatistics().getCallNb ("broadcast_size");

        // See 'Once2' for explanation of broadcast buffer size.
        if (n==1)  {  REQUIRE (bufsize >= 8 + (N*sizeof(uint32_t) + 8)*1 ); }
        else       {  REQUIRE (bufsize == 8); }
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Once2", "[once]" )
{
    size_t nbdpu = 100;

    Launcher<ArchUpmem> launcher {ArchUpmem::DPU(nbdpu), /* trace */ false, /* reset */ true};

    uint64_t truth = 0;

    size_t nbitems = 1000*1000;
    size_t N = nbdpu*nbitems;
    std::vector<uint32_t> vec(N);  for (size_t i=0; i<N; i++)  {  truth += vec[i] = i+1; }

    for (size_t n=1; n<=100; n++)
    {
        uint64_t sum = 0;
        for (auto result : launcher.run<Once1> (split(vec),n) )   {  sum += result;  }
        REQUIRE (truth==sum);

        // We check that we broadcast only once the vector
        size_t bufsize = launcher.getStatistics().getCallNb ("broadcast_size");

        // launcher.getStatistics().dump(true);

        // Here are the broadcast buffer size:
        //   for n==1:
        //      -> 8 bytes for the second parameter (same value used for all DPUs)
        //      -> for each DPU: 8 bytes for the vector length + sizeof(uint32_t)*nbitems
        //   for n>1:
        //      -> 8 bytes for the second parameter (same value used for all DPUs)

        if (n==1)  {  REQUIRE (bufsize >= 8 + (nbitems*sizeof(uint32_t) + 8)*nbdpu ); }
        else       {  REQUIRE (bufsize == 8); }
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global0", "[global]" )
{
    using type = typename Global0<ArchDummy>::type;
    Launcher<ArchUpmem>{1_dpu}.run<Global0> (type{});
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global1", "[global]" )
{
    using array_t = typename Global1<ArchMulticore>::type;

    Launcher<ArchUpmem> launcher {1_dpu};

    uint64_t truth = 0;

    array_t values;   for (size_t i=0; i<values.size(); i++)  { truth += values[i] = i+1;   }

    // IMPORTANT !!!  it is not possible to split an 'array', so the same array will be used on each process unit.
    // Trying to split an array => static assert failure.
    REQUIRE (launcher.run<Global1> (values) == truth * launcher.getProcUnitNumber());
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global2", "[global]" )
{
    using type1 = typename Global2<ArchMulticore>::type1;
    using type2 = typename Global2<ArchMulticore>::type2;

    Launcher<ArchUpmem> launcher {1_dpu};

    uint64_t truth1 = 0;
    uint64_t truth2 = 0;

    type1 v1;   for (size_t i=0; i<v1.size(); i++)  { truth1 += v1[i] = 10  + i+1;   }
    type2 v2;   for (size_t i=0; i<v2.size(); i++)  { truth2 += v2[i] = 100 + i+1;   }

    auto res = launcher.run<Global2> (v1,v2);

    REQUIRE (std::get<0>(res) == truth1 * launcher.getProcUnitNumber());
    REQUIRE (std::get<1>(res) == truth2 * launcher.getProcUnitNumber());
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global3", "[global]" )
{
    using type1 = typename Global3<ArchMulticore>::type1;
    using type2 = typename Global3<ArchMulticore>::type2;
    using type3 = typename Global3<ArchMulticore>::type3;

    Launcher<ArchUpmem> launcher {1_dpu};

    uint64_t truth1 = 0;
    uint64_t truth2 = 0;
    uint64_t truth3 = 0;

    type1 v1;   for (size_t i=0; i<v1.size(); i++)  { truth1 += v1[i] = 10   + i+1;   }
    type2 v2;   for (size_t i=0; i<v2.size(); i++)  { truth2 += v2[i] = 100  + i+1;   }
    type3 v3;   for (size_t i=0; i<v3.size(); i++)  { truth3 += v3[i] = 1000 + i+1;   }

    auto res = launcher.run<Global3> (v1,v2,v3);

    REQUIRE (std::get<0>(res) == truth1 * launcher.getProcUnitNumber());
    REQUIRE (std::get<1>(res) == truth2 * launcher.getProcUnitNumber());
    REQUIRE (std::get<2>(res) == truth3 * launcher.getProcUnitNumber());
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global4", "[global]" )
{
    Launcher<ArchUpmem> launcher {57_dpu, false};

    size_t nbitems = 1000*1000;

    uint64_t truth = 0;
    std::vector<uint32_t> v (nbitems);
    for (size_t i=0; i<v.size(); i++) { truth += (v[i] = i+1); }

    for (auto x : launcher.run<Global4> (v))  {  REQUIRE (truth == x);  }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global5", "[global]" )
{
    Launcher<ArchUpmem> launcher {57_dpu, false};

    size_t nbitems = 1000*1000;

    uint64_t truth = 0;
    std::vector<uint32_t> v (nbitems);
    for (size_t i=0; i<v.size(); i++) { truth += (v[i] = i+1); }

    // WARNING: using 'split' on a parameter tagged with 'global' in the task prototype
    // should lead to a static assertion
    launcher.run<Global5> (split(v));

    REQUIRE (true);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global6", "[global]" )
{
    Launcher<ArchUpmem> launcher {157_dpu, false};

    size_t nbitems = 1000*1000;

    uint64_t truth = 0;
    std::vector<uint32_t> v (nbitems);
    for (size_t i=0; i<v.size(); i++) { truth += (v[i] = i+1); }

    for (auto x : launcher.run<Global6> (v))  {  REQUIRE (truth == x);  }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Struct1", "[global]" )
{
    using foo = Struct1<ArchUpmem>::foo;

    Launcher<ArchUpmem> launcher {1_dpu};

    for (auto x : launcher.run<Struct1> (foo {1,2,3,4,5,6,7,8}))
    {
        REQUIRE (x.a1==1);  REQUIRE (x.a2==2);  REQUIRE (x.a3==3);  REQUIRE (x.a4==4);
        REQUIRE (x.a5==5);  REQUIRE (x.a6==6);  REQUIRE (x.a7==7);  REQUIRE (x.a8==8);
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Struct2", "[global]" )
{
    using type = MyStruct<ArchDummy>;

    Launcher<ArchUpmem> launcher {1_dpu};

    type t = { {1,2,3} };

    // no split -> each tasklet counts the same thing
    REQUIRE (launcher.run<Struct2>(t) == 6*launcher.getProcUnitNumber());
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Struct2b", "[global]" )
{
    using type = MyStruct<ArchDummy>;

    Launcher<ArchUpmem> launcher {11_dpu};

    type t = { {1,2,3} };

    // no split -> each tasklet counts the same thing
    REQUIRE (launcher.run<Struct2>(split(t)) == 6);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Struct3", "[global]" )
{
    using type = Struct3<ArchUpmem>::type;

    Launcher<ArchUpmem> launcher {1_dpu};

    type t = { {1,2,3}, {10,11,12,13}, {100,101,102,103,104} };

    // no split -> each tasklet counts the same thing
    REQUIRE (launcher.run<Struct3>(t) == (6+46+510)*launcher.getProcUnitNumber() );
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Struct4", "[global]" )
{
    using type = Struct3<ArchUpmem>::type;

    Launcher<ArchUpmem> launcher {1_dpu};

    size_t N=64*1024;
    std::array<uint64_t,3> truth = {0,0,0};

    int delta = 0;
    type t;
    for (size_t i=1; i<=1*N; i++)  { t.a1.push_back (i);  truth[0] += delta + t.a1.back(); }
    for (size_t i=1; i<=1*N; i++)  { t.a2.push_back (i);  truth[1] += delta + t.a2.back(); }
    for (size_t i=1; i<=1*N; i++)  { t.a3.push_back (i);  truth[2] += delta + t.a3.back(); }

    // no split -> each tasklet counts the same thing
    REQUIRE (launcher.run<Struct3>(t) == (truth[0]+truth[1]+truth[2])*launcher.getProcUnitNumber() );
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Struct2c", "[global]" )
{
    using type = MyStruct<ArchDummy>;

    Launcher<ArchUpmem> launcher {1_dpu};

    size_t N=1000;
    uint64_t truth = 0;

    type t;
    for (size_t i=1; i<=1*N; i++)  { t.a1.push_back (i);  truth += t.a1.back(); }

    REQUIRE (launcher.run<Struct2>(split(t)) == truth);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("GlobalOnce1", "[once]" )
{
    Launcher<ArchUpmem> launcher {15_rank};

    using type = typename GlobalOnce1<ArchMulticore>::type;

    type a;
    uint64_t truth = 0;
    for (size_t i=0; i<a.size(); i++) { truth += (a[i] = i+1); }

    for (uint32_t nbruns=1; nbruns<=100; nbruns++)
    {
        for (auto x : launcher.run<GlobalOnce1> (a, nbruns))
        {
            REQUIRE (x == truth+nbruns);
        }

        // We check that we broadcast only once the arguments tagged with 'global'
        size_t bufsize = launcher.getStatistics().getCallNb ("broadcast_size");

        if (nbruns==1)  {  REQUIRE (bufsize == sizeof(uint64_t) + sizeof(a) ); }
        else            {  REQUIRE (bufsize == sizeof(uint64_t)             ); }
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("GlobalOnce2", "[once]" )
{
    Launcher<ArchUpmem> launcher {10_rank, false};

    using array_t    = typename GlobalOnce2<ArchMulticore>::array_t;
    using value_type = typename array_t::value_type;

    array_t a;
    for (uint32_t i=0; i<a.size(); i++) {  a[i] = value_type {i+1,i+1,i+1}; }

    size_t N = a.size();

    for (uint32_t nbruns=1; nbruns<=100; nbruns++)
    {
        for (auto res : launcher.run<GlobalOnce2> (a, nbruns))
        {
            REQUIRE (res.x == (N*(N+1)/2 + 1*nbruns));
            REQUIRE (res.y == (N*(N+1)/2 + 2*nbruns));
            REQUIRE (res.z == (N*(N+1)/2 + 3*nbruns));
        }

        // We check that we broadcast only once the arguments tagged with 'global'
        size_t bufsize = launcher.getStatistics().getCallNb ("broadcast_size");

        if (nbruns==1)  {  REQUIRE (bufsize == sizeof(uint64_t) + sizeof(a) ); }
        else            {  REQUIRE (bufsize == sizeof(uint64_t)             ); }
    }
}

//////////////////////////////////////////////////////////////////////////////
template<template <typename> class TASK, typename DATATYPE>
void GlobalOnce_aux (DATATYPE&& data, size_t nbitems, uint64_t truth)
{
    Launcher<ArchUpmem> launcher {10_rank, false};

    for (uint32_t nbruns=1; nbruns<=10; nbruns++)
    {
        for (auto res : launcher.template run<TASK> (data, nbruns))
        {
            REQUIRE (res == truth*nbruns);
        }

        // We check that we broadcast only once the arguments tagged with 'global'
        size_t bufsize = launcher.getStatistics().getCallNb ("broadcast_size");
        //fmt::println("broadcast_size: {}", bufsize);

        if (nbruns==1)  {  REQUIRE (bufsize >= nbitems*sizeof(uint16_t) + 2*sizeof(uint64_t) ); }
        else            {  REQUIRE (bufsize == sizeof(uint64_t) ); }
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("GlobalOnce3", "[once]" )
{
    using type = typename GlobalOnce3<ArchDummy>::type;

    uint64_t truth = 0;
    type v;
    for (size_t i=0; i<10000; i++)  { v.push_back(i); truth += v.back(); }

    GlobalOnce_aux<GlobalOnce3> (v, v.size(), truth);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("GlobalOnce4", "[once]" )
{
    using type = GlobalOnce4Struct<ArchDummy>;

    uint64_t truth = 0;
    type v;
    for (size_t i=0; i<10000; i++)  { v.data.push_back(i); truth += v.data.back(); }

    GlobalOnce_aux<GlobalOnce4> (v, v.data.size(), truth);
}

//////////////////////////////////////////////////////////////////////////////
auto OncePadding1_aux (size_t nbDpu, size_t imax, size_t nbruns=5)
{
    std::vector<uint64_t> v1;  for (size_t i=1; i<=imax; i++)  {  v1.push_back (1+v1.size());  }
    std::vector<uint64_t> v2;  for (size_t i=1; i<=imax; i++)  {  v2.push_back (1+v2.size());  }

    Launcher<ArchUpmem> launcher {ArchUpmem::DPU{nbDpu}, false };

    for (size_t run=0; run<nbruns; run++)
    {
        for (auto [len1, sum1, sum2]: launcher.template run<OncePadding1> (split<ArchUpmem::DPU>(v1),v2))
        {
            REQUIRE (sum2 == v2.size()*(v2.size()+1)/2 );
        }

        // we add an item to v2 in order to broadcast a different vector run after run.
        v2.push_back (1+v2.size());
    }
}

TEST_CASE ("OncePadding1", "[once]" )
{
    for (size_t nbDpu : {1,2,3,5,8,13,21,34})
    {
        for (size_t imax : {11, 111, 1111, 11111})
        {
            OncePadding1_aux (nbDpu,imax);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
auto ResultsToVector_aux (size_t nbdpu)
{
    // test 'to_vector' method of the object returned by the call of Launcher::run
    // -> will get all the results in a vector

    //fmt::println ("dpu: {}", nbdpu);

    size_t nbitems = 1;

    Launcher<ArchUpmem> launcher {ArchUpmem::DPU(nbdpu)};

    auto check = [&] (auto&& results)
    {
        size_t i=1;
        for (auto&& res : results)
        {
            REQUIRE (res.size() == i);
            uint64_t s=0;  for (auto x : res) { s+=x; }
            REQUIRE (s == uint64_t{i+1}*i/2);
            i++;
        }
    };

    // Test 1: without using 'to_vector'
    check (launcher.run<ResultsToVector>(nbitems));

    // Test 2: with using 'to_vector' -> ACTIVATED BY DEFAULT
	// check (launcher.run<ResultsToVector>(nbitems).to_vector());
}

TEST_CASE ("ResultsToVector", "[misc]" )
{
    for (size_t nbdpu : {1,2,4,8,16,32,64,128,256,512,1024})
    {
        ResultsToVector_aux (nbdpu);
    }
}

//////////////////////////////////////////////////////////////////////////////
template<typename UNITS>
auto Diagnostic_aux (UNITS units, size_t imax)
{
    std::vector<uint64_t> v1;  for (size_t i=1; i<=imax; i++)  {  v1.push_back (i);  }
    std::vector<uint64_t> v2;  for (size_t i=1; i<=imax; i++)  {  v2.push_back (i);  }

    Launcher<ArchUpmem> launcher {units};

    size_t nbErrors = 0;
    for ([[maybe_unused]] auto res : launcher.template run<VectorSplitDpu> (split<ArchUpmem::DPU>(v1),v2))
    {
        nbErrors += res.second != imax*(imax+1)/2 ? 1 : 0;
    }
    //fmt::println ("launcher: {}  imax: {:6}  nbErrors: {}", fmt::join(launcher.getProcUnitDetails(),"/"), imax, nbErrors);
}

TEST_CASE ("Diagnostic", "[misc]" )
{
    for (size_t nb=1; nb<=26; nb++)  {   Diagnostic_aux (ArchUpmem::Rank(nb),11111);  }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("TemplateSpecialization", "[misc]" )
{
    for (auto res : Launcher<ArchUpmem>    {1_dpu}   .run<TemplateSpecialization> ()) { REQUIRE(res.v.size()==1); }
    for (auto res : Launcher<ArchMulticore>{1_thread}.run<TemplateSpecialization> ()) { REQUIRE(res.v.size()==0); }
}

//////////////////////////////////////////////////////////////////////////////
struct Benchmark
{
    static constexpr auto defaultCallback = [] (
            const char* taskname,
            size_t nbruns,
            auto&& launcher,
            double duration,
            auto&& input
        ) {

        double time1 = launcher.getStatistics().getTiming("run/cumul/all")      / nbruns;
        double time2 = launcher.getStatistics().getTiming("run/cumul/launch")   / nbruns;
        double time3 = launcher.getStatistics().getTiming("run/cumul/pre")      / nbruns;
        double time4 = launcher.getStatistics().getTiming("run/cumul/post")     / nbruns;
        double time5 = launcher.getStatistics().getTiming("run/cumul/result")   / nbruns;

        fmt::println ("task: {:15}  arch: {:10}  unit: {:8s} {:4} {:5}  time: {:9.5f} {:9.5f} {:9.5f} {:9.5f} {:9.5f} {:9.5f}  in: {:3}",
            taskname,
            launcher.name(),
            launcher.getTaskUnit()->name(),
            launcher.getTaskUnit()->getNbComponents(),
            launcher.getTaskUnit()->getNbUnits(),
            duration, time1, time2, time3, time4, time5,
            input
        );
    };

    template<typename...Ls>
    static auto run (const char* taskname, size_t nbruns, std::tuple<Ls...> launchersViews,
        auto inputs, auto fct
        //,auto callback = defaultCallback
    )
    {
        auto exec = [&] (auto launchers) {
            for (auto arg : inputs) {  // 'inputs' before 'launchers' => mandatory for exact cumulation times for launchers
                for (auto l : launchers) {
                    fct(taskname, nbruns, l, arg, defaultCallback);
                }
            }
        };

        [&] <std::size_t...Is> (std::index_sequence<Is...>){
            ( exec (std::get<Is>(launchersViews)), ...);
        } (std::make_index_sequence<sizeof...(Ls)>() );
    }
};

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("SyracuseReduce", "[misc]" )
{
return;
    auto fct = [] (const char* taskname, size_t nbruns, auto&& launcher, uint64_t input, auto callback) {

        std::pair<uint64_t,uint64_t> range (1, 1ULL<<input);

        auto t0 = timestamp();
        for (size_t i=0; i<nbruns; i++) {   launcher.template run<SyracuseReduce> (split(range));  }
        auto t1 = timestamp();

        callback (taskname, nbruns, launcher, (t1-t0)/nbruns/1'000'000.0, input);
    };

    auto v1 = std::views::iota(0,6) | std::views::transform([](auto nbthreadLog2) {
        return Launcher<ArchMulticore> { ArchMulticore::Thread(1<<nbthreadLog2), 32_thread};
    });

    auto v2 = std::views::iota(1,25) | std::views::transform([](auto nbrank) {
        return Launcher<ArchUpmem> { ArchUpmem::DPU(64*nbrank) };
    });

    auto input = std::views::iota(20,31);

    size_t nbruns = 5;

    Benchmark::run ("SyracuseReduce", nbruns, std::make_tuple(v2, v1), input, fct);

#if 0
    auto fct = [&] (auto idx, std::size_t nbdpu) {

        Launcher<ArchUpmem>     l1 { ArchUpmem    ::DPU   {nbdpu}    };
        Launcher<ArchMulticore> l2 { ArchMulticore::Thread{nbdpu*16}, 32_thread};

        if (l1.getProcUnitNumber() != l2.getProcUnitNumber())  {
            throw std::runtime_error("incompatible number of process units between launchers");
        }

        auto t0 = timestamp();
        [[maybe_unused]] auto res1 = l1.run<SyracuseReduce> (split(range));
        auto t1 = timestamp();
        [[maybe_unused]] auto res2 = l2.run<SyracuseReduce> (split(range));
        auto t2 = timestamp();

        double dt1 = (t1-t0)/1000.0;
        double dt2 = (t2-t1)/1000.0;

        fmt::println ("idx: {:3}  dpu: {:5}  same_result: {}  upmem: {:6.3f}  multicore: {:6.3f}  ratio: {:6.3f} ",
            idx, nbdpu, res1==res2, dt1,dt2, (dt1!=0 ? dt2/dt1 : 0)
        );
    };

    auto v = std::views::iota(1,21) | std::views::transform([](auto i) { return 64*i; });

    auto run = [] (auto v, auto fct) {
        size_t i=0;  for (auto x : v) { fct(i++,x); }
    };

    run (v,fct);
#endif
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Syracuse", "[misc]" )
{
return;
    uint64_t N = 1ULL<<30;
    std::pair<uint64_t,uint64_t> range (1, N);

//    Launcher<ArchUpmem> launcher {20_rank};
    Launcher<ArchMulticore> launcher {ArchMulticore::Thread{16*64*20}, 32_thread};
    auto result = launcher.run<Syracuse> (split(range));
}
