////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>

#include <iostream>

using namespace bpl;

using DEFAULT_ARCH = bpl::ArchMulticore;

USING (DEFAULT_ARCH);

template<class ARCH> using MySerialize = Serialize<ARCH,BufferIterator<ARCH>,1>;

//////////////////////////////////////////////////////////////////////////////////
TEST_CASE ("serialize (1)", "[Serialize]" )
{
    {
        size_t x=0;
        auto res = MySerialize<DEFAULT_ARCH>::to (x);
        REQUIRE (res.size() == sizeof(decltype(x)));
    }
    {
        double x=0;
        auto res = MySerialize<DEFAULT_ARCH>::to (x);
        REQUIRE (res.size() == sizeof(decltype(x)));
    }
    {
        tuple<char,double> x;
        auto res = MySerialize<DEFAULT_ARCH>::to (x);
        REQUIRE (res.size() == sizeof(char) + sizeof(double));
    }
    {
        tuple<char,double,tuple<int>> x;
        auto res = MySerialize<DEFAULT_ARCH>::to (x);
        REQUIRE (res.size() == sizeof(char)+sizeof(double)+sizeof(int));
    }
    {
        pair<char,double> x;
        auto res = MySerialize<DEFAULT_ARCH>::to (x);
        REQUIRE (res.size() == sizeof(char) + sizeof(double));
    }
    {
        string x ("123456789");
        auto res = MySerialize<DEFAULT_ARCH>::to (x);
        REQUIRE (res.size() == sizeof(size_t) + x.size());
    }
    {
        vector<int> x = {1,1,2,3,5,8,13};
        auto res = MySerialize<DEFAULT_ARCH>::to (x);
        REQUIRE (res.size() == sizeof(size_t) + x.size()*sizeof(int));
    }
}

//////////////////////////////////////////////////////////////////////////////////
TEST_CASE ("unserialize (common types)", "[Serialize]" )
{
    auto check = [] (auto x)  {  REQUIRE (x == MySerialize<DEFAULT_ARCH>::identity (x));  };

    check (char                             {123});
    check (size_t                           {123456});
    check (double                           {3.141592});
    check (tuple<char,double>               {127, 3.1415});
    check (tuple<char,double,tuple<int>>    {127, 3.1415, {12456} });
    check (pair<char,double>                {127, 3.1415});
    check (string                           { "123456789"});
    check (vector<int>                      {1,1,2,3,5,8,13});
    check (vector<tuple<double>>            { {1}, {2}, {3} });
    check (vector<tuple<int,double>>        { {1,1.1}, {2,2.2}, {3,3.3} });
    check (vector<vector<int>>              { {1,2}, {3,4,5}, {6,7,8,9} });
    check (vector<string>                   { "a", "bb", "ccc", "dddd", "eeeee"});
    check (vector<vector<string>>           { {"a"}, {"bb","ccc"}, {"dddd", "eeeee", "ffffff"} });
    check (array<int,7>                     { 1,2,3,4,5,6,7 });
}

#if 1
//////////////////////////////////////////////////////////////////////////////////
struct MyStruct
{
    string      name;
    vector<int> values;
    bool operator== (const MyStruct& other)  const { return name==other.name and values==other.values; }

    MyStruct (const string& n, const vector<int>& v) : name(n), values(v)  { debug ("construct");  }

    MyStruct()  { debug ("default const");  }
    MyStruct(const MyStruct& other)  { debug ("copy const");  }

    ~MyStruct()  { debug ("destructor");  }

    void debug(const char* txt)
    {
        if (false)  {  printf ("--> [MyStruct]  %15s  %p\n", txt, this);  }
    }
};

struct MyOtherStruct
{
    double x;
    vector<MyStruct> values;
    bool operator== (const MyOtherStruct& other) const  { return x==other.x and values==other.values; }
};

TEST_CASE ("unserialize (specific types)", "[Serialize]" )
{
    auto check = [] (auto&& x)  {  REQUIRE (x == MySerialize<DEFAULT_ARCH>::identity (x));  };

    static_assert (std::is_class_v<MyStruct>);
    static_assert (std::is_class_v<MyOtherStruct>);

    MyStruct s1 {"MyStruct 1",      {1,2,3,4,5}};
    check (s1);

    MyStruct s2 {"MyStruct 2",      {1,2,3,4,5,6,7}};
    check (s2);

    check (MyOtherStruct  {3.141592, {s1,s2}});
}
#endif
