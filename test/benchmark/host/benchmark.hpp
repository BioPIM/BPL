////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <common.hpp>
#include <bpl/utils/TimeUtils.hpp>

//////////////////////////////////////////////////////////////////////////////
struct Benchmark
{
    static constexpr auto defaultCallback = [] (
            const char* taskname,
            auto&& launcher,
            double duration,
            auto&& input,
            size_t nbruns=2
        ) {

        double time1 = launcher.getStatistics().getTiming("run/cumul/all")      / nbruns;
        double time2 = launcher.getStatistics().getTiming("run/cumul/launch")   / nbruns;
        double time3 = launcher.getStatistics().getTiming("run/cumul/pre")      / nbruns;
        double time4 = launcher.getStatistics().getTiming("run/cumul/post")     / nbruns;
        double time5 = launcher.getStatistics().getTiming("run/cumul/result")   / nbruns;

        fmt::println ("task: {:15}  arch: {:10}  unit: {:8s} {:4} {:5}  time: {:10.6f} {:10.6f} {:10.6f} {:10.6f} {:10.6f} {:10.6f}  in: {:3}",
            taskname,
            launcher.name(),
            launcher.getTaskUnit()->name(),
            launcher.getTaskUnit()->getNbComponents(),
            launcher.getTaskUnit()->getNbUnits(),
            duration, time1, time2, time3, time4, time5,
            input
        );
    };

    template<typename...Ls>
    static auto run (const char* taskname,  std::tuple<Ls...> launchersViews,
        auto inputs, auto fct, size_t nbruns=2
    )
    {
        auto exec = [&] (auto launchers) {
            for (auto arg : inputs) {  // 'inputs' before 'launchers' => mandatory for exact cumulation times for launchers
                for (auto l : launchers) {
                    fct(taskname, l, arg, defaultCallback, nbruns);
                }
            }
        };

        [&] <std::size_t...Is> (std::index_sequence<Is...>){
            ( exec (std::get<Is>(launchersViews)), ...);
        } (std::make_index_sequence<sizeof...(Ls)>() );
    }

    //////////////////////////////////////////////////////////////////////////////
    template<template<typename> class Task, typename...Args>
    static auto run (auto&& launcher, size_t nbruns, bool firstWithoutTime, Args&&...args)
    {
    	if (firstWithoutTime) {
    		 launcher.template run<Task> (std::forward<Args>(args)...);
    		 launcher.resetStatistics();
    	}

        auto t0 = bpl::timestamp();
        for (size_t i=0; i<nbruns; i++) {   launcher.template run<Task> (std::forward<Args>(args)...);  }
        auto t1 = bpl::timestamp();
        return (t1-t0)/nbruns/1'000'000.0;
    }
};
