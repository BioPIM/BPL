////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

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

using namespace bpl::core;
using namespace bpl::arch;

//////////////////////////////////////////////////////////////////////////////
void testRangeSplit (size_t usedRanks)
{
    std::pair<uint64_t,uint64_t> data = std::make_pair (0, 64*4096);

    auto config = ArchUpmem::Rank(usedRanks);

    Launcher<ArchUpmem> launcher (config);

    auto results = launcher.run<RangeSplit> (
        data,
        split<ArchUpmem::Rank   > (data),
        split<ArchUpmem::DPU    > (data),
        split<ArchUpmem::Tasklet> (data)
    );

    size_t nbResults  = results.size();

    size_t nbRanks    = nbResults != usedRanks*64*16 ? 0 : usedRanks;  // no multiple ranks with simulator
    size_t nbDPUs     = nbRanks==0 ? usedRanks : nbRanks*64;
    size_t nbTasklets = nbDPUs  * NR_TASKLETS;

    size_t stepRanks    = nbRanks==0 ? data.second : data.second / nbRanks;
    size_t stepDPUs     = data.second / nbDPUs;
    size_t stepTasklets = data.second / nbTasklets;

    size_t idx = 0;

    for (auto r : results)
    {
        // all
        auto c1 = std::make_pair (data.first, data.second);
        REQUIRE (std::get<0>(r).first  == c1.first);
        REQUIRE (std::get<0>(r).second == c1.second);

        // rank
        auto c2 = std::make_pair ((idx/64/16+0)*stepRanks, (idx/64/16+1)*stepRanks);
        REQUIRE (std::get<1>(r).first  == c2.first);
        REQUIRE (std::get<1>(r).second == c2.second);

        // DPU
        auto c3 = std::make_pair ((idx/16+0) *stepDPUs, (idx/16+1) *stepDPUs);
        REQUIRE (std::get<2>(r).first  == c3.first);
        REQUIRE (std::get<2>(r).second == c3.second);

        // tasklet
        auto c4 = std::make_pair ((idx/1+0)  *stepTasklets, (idx/1+1)  *stepTasklets);
        REQUIRE (std::get<3>(r).first  == c4.first);
        REQUIRE (std::get<3>(r).second == c4.second);

        if (false)
        {
            printf ("[%2ld,%2ld]  EXPECTED  [%6ld %6ld]  [%6ld %6ld]  [%6ld %6ld]  [%6ld %6ld]    ||    FOUND   [%6ld %6ld]  [%6ld %6ld]  [%6ld %6ld]  [%6ld %6ld]\n",
                idx%16, idx/16,
                c1.first, c1.second,
                c2.first, c2.second,
                c3.first, c3.second,
                c4.first, c4.second,
                std::get<0>(r).first, std::get<0>(r).second,
                std::get<1>(r).first, std::get<1>(r).second,
                std::get<2>(r).first, std::get<2>(r).second,
                std::get<3>(r).first, std::get<3>(r).second
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

    launcher.run<Checksum5> (
        split<ArchUpmem::DPU>(data)
    );
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
