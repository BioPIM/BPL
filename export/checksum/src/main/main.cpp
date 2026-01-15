////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

// We want to use the BPL API.
#include <bpl/bpl.hpp>

// We need to know the task we want to run.
#include <tasks/Checksum.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
auto compute (const std::vector<uint32_t>& data)
{
    // We configure a launcher for the chosen architecture.
    bpl::Launcher<ARCH> launcher;

    // We execute the task through the launcher.
    // NOTE: we 'split' the incoming data, therefore each process unit will see a specific part of the data.
    return launcher.template run<Checksum> (bpl::split (data));
}

////////////////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
    // We initialize a vector with some data.
    std::vector<uint32_t> v(1<<16);
    std::iota (std::begin(v), std::end(v), 1);

    // We compute the expected result.
    auto truth = std::accumulate(v.begin(), v.end(), uint64_t{0});

    // We compute the checksum on several architectures.
    uint64_t res1  = compute<bpl::ArchUpmem>     (v);
    uint64_t res2  = compute<bpl::ArchMulticore> (v);

    // We compare the results to the expected value.
    printf ("truth    : %ld\n", truth);
    printf ("UPMEM    : %ld\n", res1);
    printf ("MULTICORE: %ld\n", res2);

    // We return the result.
    return (truth==res1) and (truth==res2) ? 0 : 1;
}
