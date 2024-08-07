////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <bpl/core/Launcher.hpp>
#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>
#include <bpl/utils/RangeInt.hpp>
#include <bpl/utils/tag.hpp>

using namespace bpl::core;
using namespace bpl::arch;

#include <tasks/IntegerType.hpp>
#include <tasks/Once1.hpp>
#include <tasks/Global1.hpp>
#include <tasks/Global2.hpp>
#include <tasks/Global3.hpp>
#include <tasks/Global4.hpp>
#include <tasks/Global5.hpp>

#include <iostream>
#include <list>

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
        const once  <std::vector<uint32_t>>& a,
        const global<std::vector<uint32_t>>& b,
        int c
    )
    {  return a->size() + b->size() + c;  }
};

TEST_CASE ("Tag1", "[tag]" )
{
    using params = task_params_t<dosomething>;

    REQUIRE (hastag_once_v   <std::tuple_element_t<0,params>> == true);
    REQUIRE (hastag_once_v   <std::tuple_element_t<1,params>> == false);
    REQUIRE (hastag_once_v   <std::tuple_element_t<2,params>> == false);

    REQUIRE (hastag_global_v <std::tuple_element_t<0,params>> == false);
    REQUIRE (hastag_global_v <std::tuple_element_t<1,params>> == true);
    REQUIRE (hastag_global_v <std::tuple_element_t<2,params>> == false);
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

        if (n==1)  {  REQUIRE (bufsize == (N*sizeof(uint32_t) + 8 + 8)); }
        else       {  REQUIRE (bufsize == 8); }
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Once2", "[once]" )
{
    size_t nbdpu = 100;

    Launcher<ArchUpmem> launcher {ArchUpmem::DPU(nbdpu), /* trace */ false, /* reset */ true};

    uint64_t truth = 0;

    size_t N = nbdpu*1000*1000;
    std::vector<uint32_t> vec(N);  for (size_t i=0; i<N; i++)  {  truth += vec[i] = i+1; }

    for (size_t n=1; n<=100; n++)
    {
        uint64_t sum = 0;
        for (auto result : launcher.run<Once1> (split(vec),n) )   {  sum += result;  }
        REQUIRE (truth==sum);

        // We check that we broadcast only once the vector
        size_t bufsize = launcher.getStatistics().getCallNb ("broadcast_size");

        if (n==1)  {  REQUIRE (bufsize == 8*nbdpu + (N*sizeof(uint32_t) + 8) ); }
        else       {  REQUIRE (bufsize == 8*nbdpu); }
    }
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
