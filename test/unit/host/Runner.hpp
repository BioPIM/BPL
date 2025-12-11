////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
#include <type_traits>
#include <tuple>
#include <vector>

#include <bpl/core/Launcher.hpp>
#include <bpl/arch/ArchMulticore.hpp>
#include <bpl/arch/ArchUpmem.hpp>

////////////////////////////////////////////////////////////////////////////////

template<class...ARCHS>
class Runner
{
public:

    using arch_t   = std::conditional_t <
        sizeof...(ARCHS)==0,
        std::tuple<bpl::ArchUpmem,bpl::ArchMulticore>,
        std::tuple<ARCHS...>
    >;

    using first_t  = std::tuple_element_t<0,arch_t>;

    template<template <typename> typename TASK, typename...ARGS>
    static auto run (ARGS&&...args)
    {
        using type     = decltype(TASK<first_t>() (std::forward<ARGS>(args)...));
        using result_t = std::pair<double, std::vector<type>>;

        std::vector<result_t> allResults (std::tuple_size_v<arch_t>);

        size_t idx=0;

        std::apply ([&](auto&&...arch)
            {
               (exec<std::decay_t<decltype(arch)>,TASK,decltype(allResults[0])> (allResults[idx++], std::forward<ARGS>(args)...),...);
            },
            arch_t {}
        );

        return allResults;
    }

private:

    template<class ARCH, template <typename> typename TASK, typename result_t, typename...ARGS>
    static auto exec (result_t& result, ARGS&&...args)
    {
        double t0 = timestamp();
        for (auto&& x : bpl::Launcher<ARCH>{}.template run<TASK> (std::forward<ARGS>(args)...))  {  result.second.push_back (std::move(x));  }
        double t1 = timestamp();

        result.first = (t1-t0)/1000.0;
    }

    static inline auto  timestamp()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
};
