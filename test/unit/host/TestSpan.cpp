////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <bpl/core/Launcher.hpp>
#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>
#include <bpl/arch/ArchDummy.hpp>
#include <bpl/utils/split.hpp>

using namespace bpl;

#include <tasks/Span1.hpp>

//////////////////////////////////////////////////////////////////////////////
template<typename LAUNCHER>
void Span1_aux (LAUNCHER launcher, size_t imax)
{
    using type = typename Span1<ArchMulticore>::type;

    uint32_t truth = 0;

    std::vector<type> v(imax);

    for (size_t i=0; i<imax; i++)
    {
        uint32_t s = 0;
        for (size_t j=0; j<=i; j++)   {  v[i].push_back (j+1);   s += v[i].back(); }
        truth += s*v[i].size();
    }

    auto result = launcher.template run<Span1> (split(std::span{v}));

    REQUIRE (result == truth);
}

TEST_CASE ("Span1", "[span]" )
{
//Span1_aux (Launcher<ArchUpmem>(1_dpu,true), 16);
//return;
//
//    for (auto n : {1,2,3,5,8,13})
//    {
//        Span1_aux (Launcher<ArchMulticore> (16_thread), n);
//        Span1_aux (Launcher<ArchUpmem>     (1_dpu),     n);
//    }
}
