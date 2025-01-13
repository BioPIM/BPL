////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <bpl/core/Launcher.hpp>
#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>

#include <tasks/Mutex1.hpp>
#include <tasks/Mutex2.hpp>

using namespace bpl;

using arch_t = ArchUpmem;

////////////////////////////////////////////////////////////////////////////////
TEST_CASE ("mutex", "[synchro]" )
{
    for (uint64_t N : {1000, 10000, 100000, 1000000})
    {
        uint64_t truth = N*(N+1)/2;

        Launcher<ArchUpmem> launcher {1_dpu};

        REQUIRE (launcher.template run<Mutex1> (N) == truth);
        REQUIRE (launcher.template run<Mutex2> (N) == truth);
    }
}
