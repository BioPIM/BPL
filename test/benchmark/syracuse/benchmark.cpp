////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

// We want to use the BPL API.
#include <bpl/bpl.hpp>
#include <bpl/bank/BankRandom.hpp>
#include <bpl/utils/Range.hpp>

// We need to know the task we want to run.
#include <tasks/Benchmark1.hpp>
#include <tasks/Benchmark2.hpp>
#include <tasks/Syracuse.hpp>

using namespace bpl;

using resources_t = bpl::ArchMulticoreResources;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void bench1()
{
    const static int NBREF  = Benchmark1<resources_t>::NBREF;
    const static int NBQRY  = Benchmark1<resources_t>::NBQRY;
    const static int SEQLEN = Benchmark1<resources_t>::SEQLEN;

    RandomSequenceGenerator<SEQLEN> generator;
    BankChunk <resources_t,SEQLEN,NBREF> bankRef (generator);
    BankChunk <resources_t,SEQLEN,NBQRY> bankqry (generator);

    std::pair<int,int> range = {0, bankqry.size()};

    float first = 0.0;

    size_t nbruns = 10;

    for (int nb : {1,2,4,8,16,32,64,128,256,512,1024, 2048})
    {
        auto config = ArchUpmem::DPU(nb);

        Launcher<ArchUpmem> launcher (config);

        launcher.run<Benchmark1> (
            bankRef,
            bankqry,
            split<ArchUpmem::Tasklet,CONT>(range),
            nbruns,
            50
        );

        if (float exectime = launcher.getStatistics().getTiming ("run/launch"))
        {
            if (first==0)  { first = exectime; }

            printf ("%4d  %8.3f  %8.3f  %.3f\n",
                nb, exectime, first/exectime,
                first / (nb * exectime)
            );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
void bench2()
{
    float first = 0.0;

    uint32_t N    = 1*1024*1024;
    //uint32_t N    = 64*1024;

    std::pair<uint32_t,uint32_t> range (1, N);

    for (int nb : {1,2,4,8,16,32,64,128,256,512,1024, 2048})
    {
        auto config = ArchUpmem::DPU(nb);

        Launcher<ArchUpmem> launcher (config);

        auto res = launcher.run<Benchmark2> (split<ArchUpmem::Tasklet,CONT> (range));

        if (float exectime = launcher.getStatistics().getTiming ("run/launch"))
        {
            if (first==0)  { first = exectime; }

            printf ("%4d  %8.3f  %8.3f  %.3f   %ld\n",
                nb, exectime, first/exectime,
                first / (nb * exectime),
                res
            );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
template<typename CONFIG>
void bench3 (int N)
{
    float first = 0.0;

    Syracuse<resources_t>::range_t range (1, N);

    for (int nb : CONFIG::nbcomponents)
    {
        //printf ("*************************** %4d  ***************************\n", nb);

        auto config = typename CONFIG::component_t (nb);

        Launcher<typename CONFIG::arch_t> launcher (config);

        auto res = launcher.template run<Syracuse> (split<typename CONFIG::level_t,CONT> (range));

        //launcher.getStatistics().dump(true);

        if (float exectime = launcher.getStatistics().getTiming ("run/launch"))
        {
            if (first==0)  { first = exectime; }

            printf ("%4d  %8.3f  %8.3f  %.3f   %ld\n",
                nb, exectime, first/exectime,
                first / (nb * exectime),
                res
            );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
struct ConfigUpmem
{
    using arch_t      = ArchUpmem;
    using component_t = ArchUpmem::DPU;
    using level_t     = ArchUpmem::Tasklet;
    static constexpr int nbcomponents[] = {1,2,4,8,16,32,64,128,256,512,1024,2048};
};

struct ConfigMulticore
{
    using arch_t      = ArchMulticore;
    using component_t = ArchMulticore::Thread;
    using level_t     = ArchMulticore::Thread;
    static constexpr int nbcomponents[] = {1,2,4,8,16};
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
int main (int argc, char** argv)
{
    uint32_t N = argc >=2 ? atol(argv[1]) : 32*1024*1024;

    bench3<ConfigUpmem> (N);
    //bench3<ConfigMulticore> (N);
}
