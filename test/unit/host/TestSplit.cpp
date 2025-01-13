////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>

#include <bpl/core/Launcher.hpp>
#include <bpl/core/Task.hpp>
#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>
#include <bpl/utils/split.hpp>
#include <bpl/utils/RangeInt.hpp>

#include <tasks/RangeSplit.hpp>
#include <tasks/Checksum3.hpp>
#include <tasks/Checksum4.hpp>
#include <tasks/Checksum5.hpp>
#include <tasks/IterableSplit.hpp>
#include <tasks/SplitRangeInt.hpp>
#include <tasks/SplitDifferentSizes.hpp>
#include <tasks/VectorSplitDpu.hpp>

#include <fmt/core.h>
#include <fmt/ranges.h>

using namespace bpl;

//////////////////////////////////////////////////////////////////////////////
void testRangeSplit (size_t usedRanks)
{
    std::pair<uint64_t,uint64_t> data = std::make_pair (0, 64*4096);

    auto config = ArchUpmem::Rank(usedRanks);

    Launcher<ArchUpmem> launcher { config, false };

    auto detailsProcUnits = launcher.getProcUnitDetails();

    size_t nbRanks    = std::get<0>(detailsProcUnits);
    size_t nbDPUs     = std::get<1>(detailsProcUnits);

    auto results = launcher.run<RangeSplit> (
        data,
        split<ArchUpmem::Rank   > (data),
        split<ArchUpmem::DPU    > (data),
        split<ArchUpmem::Tasklet> (data)
    );

    size_t idx = 0;

    for (auto r : results)
    {
        // all
        auto c1 = std::make_pair (data.first, data.second);
        REQUIRE (std::get<0>(r).first  == c1.first);
        REQUIRE (std::get<0>(r).second == c1.second);

        // rank
        auto c2 = SplitOperator<decltype(data)>::split (data, idx/64/16, nbRanks);
        REQUIRE (std::get<1>(r).first  == c2.first);
        REQUIRE (std::get<1>(r).second == c2.second);

        // DPU
        auto c3 = SplitOperator<decltype(data)>::split (data, idx/16, nbDPUs);
        REQUIRE (std::get<2>(r).first  == c3.first);
        REQUIRE (std::get<2>(r).second == c3.second);

        // tasklet
        auto c4 = SplitOperator<decltype(data)>::split (c3, idx%16, 16);
        REQUIRE (std::get<3>(r).first  == c4.first);
        REQUIRE (std::get<3>(r).second == c4.second);

        if (false)
        {
            bool b1 = (std::get<1>(r).first  == c2.first) and (std::get<1>(r).second == c2.second);
            bool b2 = (std::get<2>(r).first  == c3.first) and (std::get<2>(r).second == c3.second);
            bool b3 = (std::get<3>(r).first  == c4.first) and (std::get<3>(r).second == c4.second);

            printf ("[%2ld,%2ld]  EXPECTED  [%6ld %6ld]  [%6ld %6ld]  [%6ld %6ld]  [%6ld %6ld]    ||    FOUND   [%6ld %6ld]  [%6ld %6ld]  [%6ld %6ld]  [%6ld %6ld]  ->  [%c%c%c]\n",
                idx%16, idx/16,
                c1.first, c1.second,
                c2.first, c2.second,
                c3.first, c3.second,
                c4.first, c4.second,
                std::get<0>(r).first, std::get<0>(r).second,
                std::get<1>(r).first, std::get<1>(r).second,
                std::get<2>(r).first, std::get<2>(r).second,
                std::get<3>(r).first, std::get<3>(r).second,
                b1 ? ' ' : 'X', b2 ? ' ' : 'X', b3 ? ' ' : 'X'
            );
        }

        idx++;
    }
}

TEST_CASE ("RangeSplit", "[Split]" )
{
    for (size_t usedRanks : {1, 2, 4, 8, 16})  // easier 'REQUIRE' checks if power of 2
    {
        testRangeSplit (usedRanks);
    }
}

//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("IterableSplitNo", "[Split]" )
{
    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(1));

    IntegerSequence<EvenInts> ints;

    for (auto res : launcher.run<IterableSplit> (ints))
    {
        REQUIRE (res == 65792);
    }
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("IterableSplitYes", "[Split]" )
{
    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(16));

    IntegerSequence<EvenInts> ints;

    for (__attribute__((unused)) auto res : launcher.run<IterableSplit> (split(ints)))  { }
}


//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Checksum3", "[Split]" )
{
    using buffer_t = Checksum3<ArchUpmem>::buffer_t;

    // we create a buffer and populate it.
    buffer_t data;
    for (size_t i=0; i<data.size(); i++)  { data[i] = i+1; }

    std::pair<int,int> range = std::make_pair (0, data.size());

    // we configure a launcher
    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(4));

    // we run the task
    auto result = launcher.run<Checksum3> (
        data,
        split(range)
    );

    // uint64_t cast required here.
    uint64_t truth = uint64_t(data.size())*(data.size()+1)/2;
    //printf ("#data: %ld  truth=%ld\n", data.size(), truth);
    REQUIRE (result == truth);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Checksum4", "[Split]" )
{
    Checksum4<ArchUpmem>::buffer_t data;
    for (size_t i=0; i<data.size(); i++)  { data[i] = i+1; }

    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(4));

    auto result = launcher.run<Checksum4> (
        data,
        split(std::make_pair (int(0), int(data.size())))
    );

    uint64_t truth = uint64_t(data.size())*(data.size()+1)/2;
    REQUIRE (result == truth);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Checksum5", "[Split]" )
{
    Checksum5<ArchUpmem>::buffer_t data;
    for (size_t i=0; i<data.size(); i++)  { data[i] = i+1; }

    Launcher<ArchUpmem> launcher (ArchUpmem::Rank(4));

    launcher.run<Checksum5> (data);
}

//////////////////////////////////////////////////////////////////////////////
template<typename RESOURCE>
void SplitRangeInt_aux (size_t i0, size_t i1, RESOURCE resource)
{
    using arch_t  = typename RESOURCE::arch_t;
    using level_t = typename arch_t::lowest_level_t;

    auto range = RangeInt (i0, i1);

    Launcher<arch_t> launcher {resource};

    auto result = launcher.template run<SplitRangeInt> (split<level_t>(range));

    auto truth = i1*(i1-1)/2 - i0*(i0-1)/2;

    REQUIRE (result == truth);
}

TEST_CASE ("SplitRangeInt", "[Split]" )
{
    srand(0);
    for (size_t i=1; i<=5; i++)
    {
        auto i0 = 0  + rand() % (1<<10);
        auto i1 = i0 + rand() % (1<<10);

        for (size_t nbResource : {1,10,100,500})    {  SplitRangeInt_aux (i0, i1, ArchUpmem::DPU       {nbResource} );  }
        for (size_t nbResource : {1,2,3,5,8,13,21}) {  SplitRangeInt_aux (i0, i1, ArchUpmem::Rank      {nbResource} );  }
        for (size_t nbResource : {1,2,3,5,8,13})    {  SplitRangeInt_aux (i0, i1, ArchMulticore::Thread{nbResource} );  }
    }
}

//////////////////////////////////////////////////////////////////////////////
void SplitOperator_aux (size_t nbItems, size_t nbSplits1, size_t nbSplits2)
{
    uint64_t truth=0;
    std::vector<uint32_t> v0;  for (size_t i=1; i<=nbItems; i++)  { v0.push_back(i); truth+=v0.back(); }

    uint64_t check=0;
    for (size_t i=0; i<nbSplits1; i++)
    {
        auto v1 = SplitOperator<decltype(v0)>::split (v0, i, nbSplits1);

        for (size_t j=0; j<nbSplits2; j++)
        {
            auto v2 = SplitOperator<decltype(v1)>::split (v1, j, nbSplits2);

            for (auto x : v2)
            {
                check += x;
                //printf ("[i,j]: [%3ld,%3ld]  [v1,v2]: [%3ld,%3ld]  x: %ld \n", i,j, v1.size(), v2.size(), x);
            }
        }
    }

    REQUIRE (truth == check);
}

TEST_CASE ("SplitOperator", "[Split]" )
{
    for (size_t nbItems=1; nbItems<=1000; nbItems++)
    {
        for (size_t nbSplits1 : {1,2,4,8})
        {
            for (size_t nbSplits2 : {1,2,3,5,8,13,21})
            {
                SplitOperator_aux (nbItems, nbSplits1, nbSplits2);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////

template<typename A, typename B, typename C> struct foo {  const A& a; };

template<typename A, typename B, typename C>
struct SplitOperator<foo<A,B,C>>
{
    static auto split (const foo<A,B,C>& t, std::size_t idx, std::size_t total)  { return t; }
};

TEST_CASE ("is_splitable", "[Split]" )
{
    static_assert (is_splitable_v<int>                 == false);
    static_assert (is_splitable_v<std::array<int,4>>   == false);
    static_assert (is_splitable_v<std::vector<int>>    == true);
    static_assert (is_splitable_v<std::span<int>>      == true);
    static_assert (is_splitable_v<std::pair<int,int>>  == true);

    using t1 = ::details::SplitProxy <::details::DummyLevel, SplitKind::CONT, std::pair<int,int>>;
    static_assert (is_splitable_v<t1> == true);

    static_assert (is_splitable_v<foo<char,int,long>>  == true);

}

//////////////////////////////////////////////////////////////////////////////

template<class ARCH>
struct SplitOperator<std::vector<DummyStruct<ARCH>>>
{
    static decltype(auto) split_view (std::vector<DummyStruct<ARCH>> const& t, std::size_t idx, std::size_t total)
    {
        return t[idx];
    }
};

TEST_CASE ("SplitDifferentSizes", "[Split]" )
{
    size_t nbdpus  = 16*64;
    size_t nbitems = 1000;

    using type = DummyStruct<ArchMulticore>;

    std::vector<type> vdata(nbdpus);

    // We fill several SplitDifferentSizes::type with varying number of items.
    for (size_t i=0; i<vdata.size(); i++)
    {
        auto& item = vdata[i];
        item.data.resize (nbitems*(i+1));
        for (size_t j=0; j<item.data.size(); j++) { item.data[j]=j+1; }
    }

    Launcher<ArchUpmem> launcher { ArchUpmem::DPU(nbdpus) };

    // guid is the group id, ie the group of several process units (either threads for MC or tasklets for UPMEM)

    for (auto&& [guid,res] : launcher.run<SplitDifferentSizes> (split<ArchUpmem::DPU>(vdata)))
    {
        size_t   actualNbitems = nbitems * (guid+1);
        uint64_t truth         = uint64_t(actualNbitems)*(actualNbitems+1)/2;
        REQUIRE (res == truth);
        //fmt::println ("res: {}  truth: {}  guid: {}", res, truth, guid);
    }
}

//////////////////////////////////////////////////////////////////////////////
auto VectorSplitDpu_aux (size_t nbDpu, size_t imax)
{
    std::vector<uint64_t> v1;  for (size_t i=1; i<=imax; i++)  {  v1.push_back (i);  }
    std::vector<uint64_t> v2;  for (size_t i=1; i<=imax; i++)  {  v2.push_back (i);  }

    Launcher<ArchUpmem> launcher {ArchUpmem::DPU{nbDpu} };

    for ([[maybe_unused]] auto res : launcher.template run<VectorSplitDpu> (split<ArchUpmem::DPU>(v1),v2))
    {
        REQUIRE (res.second == imax*(imax+1)/2 );
    }
}

TEST_CASE ("VectorSplitDpu", "[once]" )
{
    for (size_t nbDpu : {1,2,3,5,8,13,21,34})
    {
        for (size_t imax : {11, 111, 1111, 11111})
        {
            VectorSplitDpu_aux (nbDpu,imax);
        }
    }
}
