////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
// module: ArchUpmemKernelProfiling
// author: mmognol
////////////////////////////////////////////////////////////////////////////////

#include <cstdint>

extern "C" __attribute__((noinline)) void bpl_record_kernel_stats(
    uint64_t launch_sequence,
    uint32_t dpu_index,
    uint32_t clocks_per_sec,
    uint64_t time_all,
    uint64_t time_unserialize,
    uint64_t time_split,
    uint64_t time_exec,
    uint64_t time_result)
{   
    asm volatile(""
        :
        : "g"(launch_sequence), "g"(dpu_index),
        "g"(clocks_per_sec), "g"(time_all),
        "g"(time_unserialize), "g"(time_split),
        "g"(time_exec), "g"(time_result)); // probe: ensure function and its arguments are not optimized away
}