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

using namespace bpl;

#include <iostream>
#include <list>
#include <string_view>

//////////////////////////////////////////////////////////////////////////////
template<typename T> using is_not_reference = bpl::negation<std::is_reference>::type <T>;

TEST_CASE ("Negation", "[metaprog]" )
{
    int  a;
    int& b = a;

    static_assert (is_not_reference<decltype(a)>::value == true);
    static_assert (is_not_reference<decltype(b)>::value == false);
}

//////////////////////////////////////////////////////////////////////////////

template<typename T>
using prototype_t = bpl::FunctionSignatureParser<std::decay_t<decltype(&T::operator())>>::args_tuple;

struct FirstPredicate1  {  auto operator() (int,  int&)       {} };
struct FirstPredicate2  {  auto operator() (int&, int&, int&) {} };
struct FirstPredicate3  {  auto operator() (int&, int,  int&) {} };
struct FirstPredicate4  {  auto operator() ()                 {} };

TEST_CASE ("FirstPredicate", "[metaprog]" )
{
    REQUIRE (bpl::first_predicate_idx_v <prototype_t<FirstPredicate1>,  is_not_reference, true > == 0);
    REQUIRE (bpl::first_predicate_idx_v <prototype_t<FirstPredicate1>,  is_not_reference       > == 0);
    REQUIRE (bpl::first_predicate_idx_v <prototype_t<FirstPredicate1>, std::is_reference, false> == 1);

    REQUIRE (bpl::first_predicate_idx_v <prototype_t<FirstPredicate2>,  is_not_reference, false> == 3);
    REQUIRE (bpl::first_predicate_idx_v <prototype_t<FirstPredicate2>, std::is_reference, true > == 0);

    REQUIRE (bpl::first_predicate_idx_v <prototype_t<FirstPredicate3>,  is_not_reference, false> == 1);
    REQUIRE (bpl::first_predicate_idx_v <prototype_t<FirstPredicate3>, std::is_reference, false> == 0);

    REQUIRE (bpl::first_predicate_idx_v <prototype_t<FirstPredicate4>,  is_not_reference, false> == 0);
    REQUIRE (bpl::first_predicate_idx_v <prototype_t<FirstPredicate4>, std::is_reference, false> == 0);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE ("TransfoTuple", "[metaprog]" )
{
    static_assert (std::is_same_v<
        transform_tuple_t <
            std::tuple<int,bpl::once<float>,int>,
            bpl::removetag_once
        >,
        std::tuple<int,float,int>
    >);

    static_assert (std::is_same_v<
        transform_tuple_t <
            std::tuple<int,bpl::once<float>,int>,
            bpl::removetag_once,
            bpl::removetag_once
        >,
        std::tuple<int,float,int>
    >);

    static_assert (std::is_same_v<
        transform_tuple_t <
            std::tuple<int,bpl::once<float>,bpl::global<int> >,
            bpl::removetag_once,
            bpl::removetag_global
        >,
        std::tuple<int,float,int>
    >);

    static_assert (not std::is_same_v<
        transform_tuple_t <
            std::tuple<int,bpl::once<float>,bpl::global<int> >,
            bpl::removetag_once
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
    using type = bpl::task_params_nodecay_t<foobar>;

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

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("is_brace_constructible_v", "[metaprog]" )
{
    using at = any_type;

    struct A  {  int a; int b; int c;  };
    static_assert ( is_brace_constructible_v<A,at,at,at>);

    struct B { int a; int b; int c;  B(int a, int b, int c):a(a), b(b), c(c) {} };
    static_assert ( is_brace_constructible_v<B,at,at,at>);

    struct C { int a; int b; int c;  C () {} };
    static_assert (not is_brace_constructible_v<C,at,at,at>);

    struct D { int a; int b; int c;  D ()=default; };
    static_assert (not is_brace_constructible_v<D,at,at,at>);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("class_fields_call", "[metaprog]" )
{
    struct A { int a; int b; int c; };

    class_fields_call (A {1,2,3}, [] (int x, int y, int z)
    {
        REQUIRE (x==1);
        REQUIRE (y==2);
        REQUIRE (z==3);
    });

    class B {  public:  int a; int b; int c;  };
    class_fields_call (B { 1,2,3 }, [] (int x, int y, int z)
    {
        REQUIRE (x==1);
        REQUIRE (y==2);
        REQUIRE (z==3);
    });

    class C {  public:  C(int a, int b, int c):a(a), b(b), c(c) {}   int a; int b; int c;  };
    class_fields_call (C (1,2,3), [] (int x, int y, int z)
    {
        REQUIRE (x==1);
        REQUIRE (y==2);
        REQUIRE (z==3);
    });

    class D {  public:  int a=1; int b=2; int c=3; };
    class_fields_call (D {}, [] (int x, int y, int z)
    {
        REQUIRE (x==1);
        REQUIRE (y==2);
        REQUIRE (z==3);
    });
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("class_fields_iterate", "[metaprog]" )
{
    struct foo { int a; int b; int c; };

    size_t idx=0;
    class_fields_iterate (foo {1,2,3}, [&idx] (auto&& x)
    {
        if (idx==0)  { REQUIRE (x==1); }
        if (idx==1)  { REQUIRE (x==2); }
        if (idx==2)  { REQUIRE (x==3); }
        idx++;
    });
    REQUIRE (idx==3);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("class_fields_iterate many", "[metaprog]" )
{
    struct foo { int p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12,p13,p14,p15; };
    int idx=0;
    class_fields_iterate (foo {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}, [&idx] (auto&& x)   {  REQUIRE (x==++idx); });
    REQUIRE (idx==15);

    struct bar { int n; std::string s;  std::array<char,3> a;  };
    class_fields_iterate (bar {100, "hello", {1,2,3} }, [] (auto&& x)   {   });
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("class_fields_iterate array", "[metaprog]" )
{
    struct foo  {  char data[4];  };

    // doesn't work yet
    //class_fields_iterate (foo{}, [] (auto&& field)  { });

    struct bar  {  std::array<uint8_t,4> data;  };

    class_fields_iterate (bar{}, [] (auto&& field)
    {
        REQUIRE (field.size()==4);
    });
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("to_tuple", "[metaprog]" )
{
    struct foo { int a; int b; int c; };

    constexpr auto t = to_tuple (foo {1,2,3});

    static_assert (std::tuple_size_v<decltype(t)> == 3);
    static_assert (std::get<0>(t) == 1);
    static_assert (std::get<1>(t) == 2);
    static_assert (std::get<2>(t) == 3);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("CTMap1", "[metaprog]" )
{
    constexpr CTMap<int, int, 3>  m1 {{{ { 10, 20 }, { 11, 21 }, { 23, 7 } }}};
    static_assert (m1[10] == 20, "Error.");
    static_assert (m1[11] == 21, "Error.");
    static_assert (m1[23] ==  7, "Error.");

    constexpr CTMap<std::string_view, int, 2>  m2 {{{ {"ten",10}, {"one",1} }}};
    static_assert (m2["ten"] == 10, "Error.");

    constexpr IntMap<1> m3 {{{ {"foo",321} }}};
    static_assert (m3["foo"] == 321, "Error.");

    constexpr Properties m4 {{{ {"foo",4321} }}};
    static_assert (m4["foo"] == 4321, "Error.");
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("CTMap2", "[metaprog]" )
{
    constexpr Properties p1 {{{ {"A",1},  {"B",2}, {"C", 3 } }}};
    constexpr Properties p2 {{{ {"A",11},          {"C",13 }, {"D",14} }}};

    constexpr auto p = merge (p1, p2);

    static_assert (p["A"] ==  1, "Error.");
    static_assert (p["B"] ==  2, "Error.");
    static_assert (p["C"] ==  3, "Error.");
    static_assert (p["D"] == 14, "Error.");
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Log2", "[metaprog]" )
{
    #define TL2(n,v)  static_assert ( (Log2<n>::value==v) && (Log2Ext<n>::value==v))

    TL2(1,0);   TL2(2,1);   TL2(3,1);   TL2(4,2);   TL2(5,2);   TL2(6,2);   TL2(7,2);   TL2(8,3);   TL2(9,3);   TL2(10,3);
    TL2(15,3);  TL2(16,4);  TL2(17,4);
    TL2(31,4);  TL2(32,5);  TL2(33,5);

    static_assert (Log2Ext<0>::value==0);

    #undef TL2
}

