////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <catch2/catch_test_macros.hpp>

#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>

using namespace bpl;

////////////////////////////////////////////////////////////////////////////////
TEST_CASE ("MULTICORE: check properties", "[Arch]" )
{
    REQUIRE (ArchMulticore().name() == "multicore");
    REQUIRE (ArchMulticore( ).getProcUnitNumber() ==  1);
    REQUIRE (ArchMulticore(8).getProcUnitNumber() ==  8);
}

////////////////////////////////////////////////////////////////////////////////
TEST_CASE ("UPMEM: check properties", "[Arch]" )
{
    REQUIRE (ArchUpmem().name() == "upmem");

    // One rank -> 64 DPU with 16 tasklets
    //REQUIRE (ArchUpmem().getProcUnitNumber() ==  64*16);
}
