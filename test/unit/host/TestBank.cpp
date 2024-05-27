////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <bpl/bank/BankFasta.hpp>
#include <bpl/bank/BankRandom.hpp>
#include <bpl/bank/BankChunk.hpp>

#include <bpl/core/Launcher.hpp>
#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>

#include <bpl/utils/serialize.hpp>
#include <bpl/utils/BufferIterator.hpp>

#include <tasks/Bank1.hpp>
#include <tasks/Bank2.hpp>
#include <tasks/Bank3.hpp>
#include <tasks/Bank4.hpp>
#include <tasks/Bank5.hpp>

using namespace bpl::bank;
using namespace bpl::arch;

using resources_t = bpl::arch::ArchMulticoreResources;
using Serializer  = bpl::core::Serialize<resources_t,bpl::core::BufferIterator<resources_t>,8>;

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Bank1", "[Bank]" )
{
    using resources_t = bpl::arch::ArchMulticoreResources;

    RandomSequenceGenerator<32> generator;
    BankChunk <resources_t, decltype(generator)::SIZE> bankOut (generator);

    Launcher<ArchUpmem> launcher (4_rank);

    std::pair<int,int> range = {0, bankOut.sequences_.size()};

    auto result = launcher.run<Bank1> (
        bankOut.sequences_,
        range
        //split<ArchUpmem::Tasklet,CONT>(range)
    );
}

//////////////////////////////////////////////////////////////////////////////
template<template<typename> class TASK>
auto TestBank_aux (size_t nbDPU)
{
    using resources_t = bpl::arch::ArchMulticoreResources;

    const static int SEQNB  = TASK<resources_t>::SEQNB;
    const static int SEQLEN = TASK<resources_t>::SEQLEN;

    RandomSequenceGenerator<SEQLEN> generator;
    BankChunk <resources_t,SEQLEN,SEQNB> bankOut (generator);

    std::pair<int,int> range = {0, bankOut.size()};

    // We compute on the Host first.
    auto truth = TASK<resources_t>() (bankOut.sequences_, range);
    //printf ("TRUTH : %4d %4d %4d %4d\n", std::get<0>(truth), std::get<1>(truth), std::get<2>(truth), std::get<3>(truth));

    Launcher<ArchUpmem> launcher { ArchUpmem::DPU(nbDPU), false };

    auto res =  launcher.run<TASK> (
        bankOut.sequences_,
        split (range)
    );

    return make_pair (truth,res);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Bank2", "[Bank]" )
{
    auto results = TestBank_aux <Bank2> (1);

    auto t = results.first;

    for (auto r : results.second)
    {
        std::get<0>(t) -= std::get<0>(r);
        std::get<1>(t) -= std::get<1>(r);
        std::get<2>(t) -= std::get<2>(r);
        std::get<3>(t) -= std::get<3>(r);
    }

    REQUIRE (std::get<0>(t)==0);
    REQUIRE (std::get<1>(t)==0);
    REQUIRE (std::get<2>(t)==0);
    REQUIRE (std::get<3>(t)==0);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Bank3", "[Bank]" )
{
    size_t k=64;

    auto result = TestBank_aux <Bank3> (k);
    auto truth  = result.first;
    auto res    = result.second;

    REQUIRE (std::get<0>(res) == std::get<0>(truth));
    REQUIRE (std::get<1>(res) == std::get<1>(truth));
    REQUIRE (std::get<2>(res) == std::get<2>(truth));
    REQUIRE (std::get<3>(res) == std::get<3>(truth));
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Bank4", "[Bank]" )
{
    using resources_t = bpl::arch::ArchMulticoreResources;

    RandomSequenceGenerator<32> generator;
    BankChunk <resources_t,decltype(generator)::SIZE> bankOut (generator);

    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1));

    auto result = launcher.run<Bank4> (bankOut);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("Bank5", "[Bank]" )
{
    using resources_t = bpl::arch::ArchMulticoreResources;

    RandomSequenceGenerator<32> generator;
    BankChunk <resources_t,decltype(generator)::SIZE> bankOut (generator);

    Launcher<ArchUpmem> launcher (ArchUpmem::DPU(1));

    auto result = launcher.run<Bank5> (bankOut);
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("BankRandom1", "[Bank]" )
{
//    BankRandom bank (1024, 64);
//
//    bank.iterate ([] (size_t idxSeq, auto&& seq)
//    {
//        printf ("[%4d] ", idxSeq);
//        seq.iterate ([] (size_t i, uint8_t c)  { printf ("%c", c); });
//        printf ("\n");
//    });
//
//    auto buffer = Serializer::to (bank);
//    printf ("#buffer=%ld  is_trivially_copiable=%d  is_arithmetic_v=%d  is_iterable=%d \n",
//        buffer.size(),
//        std::is_trivially_copyable_v<BankRandom>,
//        std::is_arithmetic_v<BankRandom>,
//        bpl::core::is_iterable<BankRandom>::value
//    );
}

//////////////////////////////////////////////////////////////////////////////
TEST_CASE ("BankChunk1", "[Bank]" )
{
//    BankRandom bankIn (1024, 64);
//
//    BankChunk<resources_t,256> bankOut;
//
//    auto inBegin = bankIn.begin();
//    auto inEnd   = bankIn.end();
//
//    auto outBegin = bankOut.begin();
//    auto outEnd   = bankOut.end();
//
//    for ( ; inBegin!=inEnd and outBegin!=outEnd; ++inBegin, ++outBegin)  {  *(outBegin) = *(inBegin);  }
//
//    for (auto& seq : bankOut)
//    {
//        seq.iterate ([] (size_t i, uint8_t c)  { printf ("%c", c); });  printf("\n");
//    }
//
//    auto buffer = Serializer::to (bankOut);

}
