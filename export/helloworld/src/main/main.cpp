////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

// We want to use the BPL API.
#include <bpl/bpl.hpp>

// We need to know the task we want to run.
#include <tasks/HelloWorld.hpp>

using namespace bpl;

////////////////////////////////////////////////////////////////////////////////
struct config_upmem
{
    using arch_t  = ArchUpmem;
    using level_t = ArchUpmem::DPU;
    using unit_t  = ArchUpmem::Tasklet;
    static const int nbunits = 1;
};

struct config_multicore
{
    using arch_t  = ArchMulticore;
    using level_t = ArchMulticore::Thread;
    using unit_t  = ArchMulticore::Thread;
    static const int nbunits = 16*config_upmem::nbunits;
};

////////////////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
    // We define our target architecture with the 'ArchUpmem' configuration.
    using config = config_upmem;

    // We configure a launcher for the chosen architecture.
    Launcher<config::arch_t> launcher {config::level_t(config::nbunits)};

    // We initiate an array with some data.
    std::array<char,16> data {'H','e','l','l', 'o', ' ', 'W','o','r','l','d', ' ', '!', 0};

    // We execute the 'HelloWorld' task through the launcher.
    auto results = launcher.run<HelloWorld> (data);

    // We dump each sub result (one per process unit)
    size_t idx = 0;
    for (auto result : results)
    {
        printf ("process unit %4ld computed result: %d\n", idx++, result);
    }

    // We return the result.
    return 0;
}
