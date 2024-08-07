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

#include <iostream>
#include <list>

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

////////////////////////////////////////////////////////////////////////////////
TEST_CASE ("TransfoTuple", "[metaprog]" )
{
    static_assert (std::is_same_v<
        transform_tuple_t <
            std::tuple<int,bpl::core::once<float>,int>,
            bpl::core::removetag_once
        >,
        std::tuple<int,float,int>
    >);

    static_assert (std::is_same_v<
        transform_tuple_t <
            std::tuple<int,bpl::core::once<float>,int>,
            bpl::core::removetag_once,
            bpl::core::removetag_once
        >,
        std::tuple<int,float,int>
    >);

    static_assert (std::is_same_v<
        transform_tuple_t <
            std::tuple<int,bpl::core::once<float>,bpl::core::global<int> >,
            bpl::core::removetag_once,
            bpl::core::removetag_global
        >,
        std::tuple<int,float,int>
    >);

    static_assert (not std::is_same_v<
        transform_tuple_t <
            std::tuple<int,bpl::core::once<float>,bpl::core::global<int> >,
            bpl::core::removetag_once
        >,
        std::tuple<int,float,int>
    >);
}

//////////////////////////////////////////////////////////////////////////////
template<typename T>  struct transfo_const_vector2list                          {  using type = T; };
template<typename T>  struct transfo_const_vector2list <const std::vector<T>>   {  using type = const std::list<T>;   };
template<typename T>  struct transfo_const_vector2list <const std::vector<T>&>  {  using type = const std::list<T>&;  };

TEST_CASE ("TransfoType", "[metaprog]" )
{
    static_assert (std::is_same_v<
        transfo_const_vector2list<const std::vector<int>>::type,
        const std::list<int>
    >);

    static_assert (std::is_same_v<
        transfo_const_vector2list<const std::vector<int>&>::type,
        const std::list<int>&
    >);
}

//////////////////////////////////////////////////////////////////////////////
struct foobar {  auto operator()  (int, const int, const int&) {}  };

TEST_CASE ("TaskParameters", "[metaprog]" )
{
    using type = bpl::core::task_params_nodecay_t<foobar>;

    static_assert (std::is_same_v <std::tuple_element_t<0,type>, int>);
    static_assert (std::is_same_v <std::tuple_element_t<1,type>, int>);
    static_assert (std::is_same_v <std::tuple_element_t<2,type>, const int&>);
}

//////////////////////////////////////////////////////////////////////////////
template<typename A, typename B> struct comp1 : std::true_type  {};
template<typename A, typename B> struct comp2 : std::false_type {};

TEST_CASE ("TuplesCompatibility", "[metaprog]" )
{
    using A = std::tuple<int,float>;
    using B = std::tuple<int,double>;

    static_assert (compare_tuples_v<comp1,A,B> == true);
    static_assert (compare_tuples_v<comp2,A,B> == false);
}
