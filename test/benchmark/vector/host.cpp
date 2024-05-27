////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

extern "C"
{
#include <dpu.h>
}

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <chrono>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
static double timestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
std::pair<uint32_t,double> process (int nmax_log2, uint32_t choice)
{
    uint32_t nb_runs  = 0;
    uint32_t nb_items = 0;

    struct dpu_set_t set, dpu;

    // PLEASE set the following variable:
    //     DPU_BINARY=/home/edrezen/git/org.pim.bpl/build/test/benchmark/vector/benchmark_dpu_vector
    char dpubinary[128];  snprintf (dpubinary, sizeof(dpubinary), "%s.%d", getenv("DPU_BINARY"), nmax_log2);

    DPU_ASSERT (dpu_alloc(1, NULL, &set));
    DPU_ASSERT (dpu_load(set, dpubinary, NULL));

    DPU_ASSERT(dpu_broadcast_to(set, "choice", 0, (uint8_t *)&choice, sizeof(choice), DPU_XFER_DEFAULT));

    double t0 = timestamp();
    DPU_ASSERT (dpu_launch(set, DPU_SYNCHRONOUS));
    double t1 = timestamp();

    DPU_FOREACH(set, dpu)
    {
        DPU_ASSERT(dpu_copy_from(dpu, "nb_items", 0, (uint8_t *)&nb_items, sizeof(nb_items)));
        DPU_ASSERT(dpu_copy_from(dpu, "nb_runs",  0, (uint8_t *)&nb_runs,  sizeof(nb_runs )));
        //DPU_ASSERT(dpu_log_read (dpu, stdout));
    }

    DPU_ASSERT(dpu_free(set));

    double ts   = (t1-t0)/1000.0/nb_runs;

    return std::make_pair (nb_items, ts);
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
    int n0 = argc>=2 ? atoi(argv[1]) :  9;
    int n1 = argc>=3 ? atoi(argv[2]) : 22;

    auto rate = [] (size_t nblog2, double ts) {  return double(1ULL << nblog2) / ts / 1000000.0;  };

    printf ("%13s  %13s  %13s  %13s\n", "log2(NMAX)", "vector", "handcrafted", "upmem");

    if (true)
    {
        for (int n=n0; n<=n1; n++)
        {
            auto w1 = process (n, 0+1);
            auto w2 = process (n, 0+2);
            auto w3 = process (n, 0+3);

            printf ("%13d  %13.5f  %13.5f  %13.5f\n", w1.first,
                rate(w1.first,          w1.second),  rate(w2.first,          w2.second), rate(w3.first,          w3.second)
            );
        }
    }
    else
    {
        for (int n=n0; n<=n1; n++)
        {
            auto w1 = process (n, 0+1);
            auto w2 = process (n, 0+2);
            auto w3 = process (n, 0+3);

            auto i1 = process (n, 3+1);
            auto i2 = process (n, 3+2);
            auto i3 = process (n, 3+3);

            printf ("%13d  %13.5f  %13.5f  %13.5f\n", w1.first,
                rate(w1.first,i1.second-w1.second),  rate(w2.first,i2.second-w2.second), rate(w3.first,i3.second-w3.second)
            );
        }
    }

    return 0;
}
