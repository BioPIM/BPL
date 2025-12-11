#include <dpu.h>

#define DPU_BINARY "dpu"
#define NB_DPUS    1

////////////////////////////////////////////////////////////////////////////////
static uint32_t buf0[8];
static uint32_t buf1[576];  // works for values <=574

////////////////////////////////////////////////////////////////////////////////
bool get_block (struct sg_block_info *out, uint32_t dpu_index, uint32_t block_index, void *args)
{
    uint64_t* checksum = (uint64_t*)args;

    bool result = true;

    switch (block_index)
    {
    case 0:
    {
        out->length = sizeof(buf0);
        out->addr   = (uint8_t*)buf0;
        for (int i=0; i<out->length; i++)  { *checksum += (out->addr)[i]; }
        break;
    }
    case 1:
    {
        out->length = sizeof(buf1);
        out->addr   = (uint8_t*)buf1;
        for (int i=0; i<out->length; i++)  { *checksum += (out->addr)[i]; }
        break;
    }
    default:
        result = false;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
void init (uint32_t* buf, int nb)
{
    for (size_t i=0; i<nb; i++)  { buf[i] = i+1; }
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    int nbdpu = argc>=2 ? atoi(argv[1]) : DPU_ALLOCATE_ALL;

    init (buf0, sizeof(buf0)/sizeof(buf0[0]));
    init (buf1, sizeof(buf1)/sizeof(buf1[0]));

    uint64_t checksumHost = 0;
    uint32_t nbBlocks  = 0;
    uint32_t size      = 0;
    uint32_t idxGlobal = 0;
    for (size_t block_index=0; ; block_index++, nbBlocks++)
    {
        struct sg_block_info out;

        if (get_block (&out, 0, block_index, &checksumHost)==false) { break; }

        size += out.length;

        for (uint32_t idx=0; idx<out.length; idx++, idxGlobal++)
        {
            if (idxGlobal%32==0)  {  printf ("HOST  [%4d]  ", idxGlobal); }

            printf ("%3d%c", (out.addr)[idx], (idxGlobal%32==31 ? '\n' : (idxGlobal%4==3 ? '|' : ' ')));
        }
    }

    printf ("=============================================================================\n");
    printf ("HOST: checksum: %ld  (nbBlocks: %ld  size: %ld  nbdpu: %d) \n", checksumHost, nbBlocks, size, nbdpu);

    struct dpu_set_t set,dpu;

    /* Initialize and run */
    DPU_ASSERT(dpu_alloc(nbdpu, "sgXferEnable=true,sgXferMaxBlocksPerDpu=8", &set));
    DPU_ASSERT(dpu_load (set, DPU_BINARY, NULL));

    dpu_broadcast_to(set, "size", 0, &size, sizeof(size), DPU_XFER_DEFAULT);

    get_block_t block_info = { .f = get_block,  .args = NULL, .args_size = 0 };

    DPU_ASSERT (dpu_push_sg_xfer (
        set,
        DPU_XFER_TO_DPU,
        "buffer",
        0,
        size,
        &block_info,
        DPU_SG_XFER_DEFAULT
    ));

    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

    size_t nberrors = 0;

    DPU_FOREACH(set, dpu)
    {
        uint64_t checksums[NR_TASKLETS];

        DPU_ASSERT(dpu_copy_from(dpu, "checksums", 0, checksums, sizeof(checksums)));
        for (size_t i=0; i<NR_TASKLETS; i++)
        {
            if (checksums[i] != checksumHost)  { printf ("ERROR[%2d] %ld\n", i, checksums[i]);  nberrors++; }
        }
    }

//    DPU_FOREACH(set, dpu) {  DPU_ASSERT(dpu_log_read(dpu, stdout));  }

    DPU_ASSERT(dpu_free(set));

    return nberrors;
}
