extern "C"
{
    #include <dpu.h>
}

#define DPU_BINARY "dpu"

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    int nbdpu = argc>=2 ? atoi(argv[1]) : 1;
    struct dpu_set_t set,dpu;

    DPU_ASSERT(dpu_alloc(nbdpu, "sgXferEnable=true", &set));
    DPU_ASSERT(dpu_load (set, DPU_BINARY, NULL));

    DPU_ASSERT(dpu_launch(set, DPU_SYNCHRONOUS));

    DPU_FOREACH(set, dpu) {  DPU_ASSERT(dpu_log_read(dpu, stdout));  }

    DPU_ASSERT(dpu_free(set));

    return 0;
}
