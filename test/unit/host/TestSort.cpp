////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <common.hpp>

#include <fmt/core.h>

#include <bpl/utils/MergeUtils.hpp>
#include <bpl/utils/RandomUtils.hpp>

#include <tasks/SortSelectionVector.hpp>
#include <tasks/SortSelectionVectorView.hpp>

using namespace bpl;

////////////////////////////////////////////////////////////////////////////////
template<template<typename> class TASK>
auto SortSelection_aux(auto&& launcher, auto const& v)
{
    using value_type = typename std::decay_t<decltype(v)>::value_type;

     auto timestamp = []  {
         return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() / 1000.0;
     };

    [[maybe_unused]] auto t0 = timestamp();
    auto results = launcher.template run<TASK> (split(v));
    [[maybe_unused]] auto t1 = timestamp();

    value_type i=1;
    size_t nbok=0;

    // We sort-merge the N partial results and check that we have integers from 1 to N
    merge (results, [&] (auto&& x) {  nbok += (i++ == x) ? 1 : 0; });
    REQUIRE (nbok==v.size());

    [[maybe_unused]] auto t2 = timestamp();
    //fmt::println ("arch: {:10}  #items: {:9}  #result: {:6}  time: {:5.3} {:5.3}", launcher.name(), v.size(), results.size(), t1-t0, t2-t1);

    return t1-t0;
}

template<template<typename> class TASK>
auto SortSelection_aux_aux (auto&& launcher, size_t nbtasklets, size_t nbitemsPerTasklet)
{
    using value_type = typename TASK<ArchMulticore>::value_type;

    auto v = get_random_permutation<value_type> (nbtasklets*nbitemsPerTasklet);

    return std::make_pair (SortSelection_aux<TASK> (launcher, v), nbtasklets*nbitemsPerTasklet);
}

TEST_CASE ("SortSelectionSimple", "[sort]" )
{
    Launcher<ArchUpmem> launcher {1_dpu,false};
    SortSelection_aux_aux<SortSelectionVector>  (launcher, launcher.getProcUnitNumber(), 130);
}

TEST_CASE ("SortSelection", "[sort]" )
{
    constexpr bool full = false;
    size_t r1 = 20;
    size_t r0 = full ? r1 : r1;

    for (size_t nbitemsPerPu : {1<<4, 1<<6, 1<<8, 1<<10, 1<<12 })
    {
        for (size_t nbranks=r0; nbranks<=r1; nbranks++)
        {
            size_t nbdpu      = 64*nbranks;
            size_t nbtasklets = 16*nbdpu;

            auto launchers = std::make_tuple (
                Launcher<ArchMulticore>  {ArchMulticore::Thread{nbtasklets}, 32_thread},
                Launcher<ArchUpmem>      {ArchUpmem::DPU{nbdpu},false,true}
            );

            REQUIRE (std::get<0>(launchers).getProcUnitNumber() == std::get<1>(launchers).getProcUnitNumber());

            [[maybe_unused]] auto [t0,nbi0] = SortSelection_aux_aux<SortSelectionVector> (std::get<0>(launchers), nbtasklets, nbitemsPerPu);
            [[maybe_unused]] auto [t1,nbi1] = SortSelection_aux_aux<SortSelectionVector> (std::get<1>(launchers), nbtasklets, nbitemsPerPu);

            if (full) {
                auto& stats = std::get<1>(launchers).getStatistics();

                fmt::println ("[rank,dpu,tasklet]: {:2} {:4} {:6}  nbitemsPerPu: {:5}   multicore: {:6.3f}   upmem: {:6.3f}  ratio: {:6.3f}   PIM get/read: {:6} {:10}   upmem::launch {:} ",
                    nbranks, nbdpu, nbtasklets, nbitemsPerPu,
                    t0, t1, t1/t0,
                    stats.getTag("resources/MRAM/calls/get"),
                    stats.getTag("resources/MRAM/calls/read"),
                    stats.getTiming("run/launch")
                );
                //stats.dump(true);
            }
        }
    }
}
