////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

// We want to use the BPL API.
#include <bpl/bpl.hpp>

// We need to know the task we want to run.
#include <tasks/Checksum.hpp>

using namespace bpl::core;
using namespace bpl::arch;

////////////////////////////////////////////////////////////////////////////////
struct config_upmem
{
    using arch_t  = ArchUpmem;
    using level_t = ArchUpmem::Rank;
    using unit_t  = ArchUpmem::Tasklet;
    static const int nbunits = 4;
};

struct config_multicore
{
    using arch_t  = ArchMulticore;
    using level_t = ArchMulticore::Thread;
    using unit_t  = ArchMulticore::Thread;
    static const int nbunits = 16;
};

////////////////////////////////////////////////////////////////////////////////
template<typename CONFIG>
auto compute (const std::vector<uint32_t>& data)
{
    // We configure a launcher for the chosen architecture.
    Launcher<typename CONFIG::arch_t> launcher { typename CONFIG::level_t(CONFIG::nbunits)};

    // We execute the task through the launcher.
    // NOTE: we 'split' the incoming data, therefore each process unit will see a specific part of the data.
    return launcher.template run<Checksum> (split (data));
}

////////////////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
    size_t N = 1UL << 24;

    // We initialize a vector with some data.
    uint64_t truth = 0;
    std::vector<uint32_t> data (N);
    for (size_t i=0; i<data.size(); i++)  { truth += (data[i]=i); }

    // We compute the checksum on several architectures.
    uint64_t res1  = compute<config_upmem>     (data);
    uint64_t res2  = compute<config_multicore> (data);

    // We compare the different results.
    printf ("truth    : %ld\n", truth);
    printf ("UPMEM    : %ld\n", res1);
    printf ("MULTICORE: %ld\n", res2);

    // We return the result.
    return (truth==res1) and (truth==res2) ? 0 : 1;
}
