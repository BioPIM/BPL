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

#include <iostream>

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
template<typename T> using is_not_reference = bpl::core::negation<std::is_reference>::type <T>;

TEST_CASE ("Negation", "[metaprog]" )
{
    int  a;
    int& b = a;

    static_assert (is_not_reference<decltype(a)>::value == true);
    static_assert (is_not_reference<decltype(b)>::value == false);
}

//////////////////////////////////////////////////////////////////////////////

template<typename T>
using prototype_t = bpl::core::FunctionSignatureParser<std::decay_t<decltype(&T::operator())>>::args_tuple;

struct FirstPredicate1  {  auto operator() (int,  int&)       {} };
struct FirstPredicate2  {  auto operator() (int&, int&, int&) {} };
struct FirstPredicate3  {  auto operator() (int&, int,  int&) {} };
struct FirstPredicate4  {  auto operator() ()                 {} };

TEST_CASE ("FirstPredicate", "[metaprog]" )
{
    REQUIRE (bpl::core::first_predicate_idx_v <prototype_t<FirstPredicate1>,  is_not_reference, true > == 0);
    REQUIRE (bpl::core::first_predicate_idx_v <prototype_t<FirstPredicate1>,  is_not_reference       > == 0);
    REQUIRE (bpl::core::first_predicate_idx_v <prototype_t<FirstPredicate1>, std::is_reference, false> == 1);

    REQUIRE (bpl::core::first_predicate_idx_v <prototype_t<FirstPredicate2>,  is_not_reference, false> == 3);
    REQUIRE (bpl::core::first_predicate_idx_v <prototype_t<FirstPredicate2>, std::is_reference, true > == 0);

    REQUIRE (bpl::core::first_predicate_idx_v <prototype_t<FirstPredicate3>,  is_not_reference, false> == 1);
    REQUIRE (bpl::core::first_predicate_idx_v <prototype_t<FirstPredicate3>, std::is_reference, false> == 0);

    REQUIRE (bpl::core::first_predicate_idx_v <prototype_t<FirstPredicate4>,  is_not_reference, false> == 0);
    REQUIRE (bpl::core::first_predicate_idx_v <prototype_t<FirstPredicate4>, std::is_reference, false> == 0);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global1", "[global]" )
{
    using array_t = typename Global1<ArchMulticore>::type;

    Launcher<ArchUpmem> launcher {1_dpu};

    uint64_t truth = 0;

    array_t values;   for (size_t i=0; i<values.size(); i++)  { truth += values[i] = i+1;   }

    REQUIRE (launcher.run<Global1> (values) == truth * launcher.getProcUnitNumber());
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Global2", "[global]" )
{
    using array_t = typename Global1<ArchMulticore>::type;

    Launcher<ArchUpmem> launcher {1_dpu};

    uint64_t truth = 0;

    array_t values;   for (size_t i=0; i<values.size(); i++)  { truth += values[i] = i+1;   }

    // IMPORTANT !!!  it is not possible to split an 'array', so the same array will be used on each process unit.
    auto result = launcher.run<Global1> (split(values));

    REQUIRE (result == truth * launcher.getProcUnitNumber());
}
