 
#include <mram.h>
#include <stdio.h>
#include <defs.h>

#define DPU_BUFFER_SIZE (1<<20)

__host        uint32_t size;
__mram_noinit uint8_t  buffer [DPU_BUFFER_SIZE];
__host        uint64_t checksums[NR_TASKLETS];

int main (void)
{
    uint64_t checksum = 0;
    for (uint32_t idx=0; idx<size; idx++)
    {
        checksum += buffer[idx];
    }

    checksums[me()] = checksum;

    return 0;
}
