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

    template<template <typename> typename TASK, typename...ARGS>
    static auto run (ARGS&&...args)  {
        // We might have task results of different types wrt the architecture.
        // So we return a tuple instead of a vector.
        return std::apply ([&](auto&&...arch) {
            return std::make_tuple (
                exec<std::decay_t<decltype(arch)>,TASK> (std::forward<ARGS>(args)...)...
            ); },
            arch_t {}
        );
    }

private:

    template<class ARCH, template <typename> typename TASK, typename...ARGS>
    static auto exec (ARGS&&...args)
    {
        double t0 = timestamp();
        auto results = bpl::Launcher<ARCH>{}.template run<TASK> (std::forward<ARGS>(args)...);
        double t1 = timestamp();
        return std::make_pair ((t1-t0)/1000.0, std::move(results));
    }

    static inline auto  timestamp()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    }
};
