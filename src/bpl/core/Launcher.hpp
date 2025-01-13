////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/utils/metaprog.hpp>
#include <bpl/utils/reduce.hpp>
#include <bpl/utils/splitter.hpp>

#include <tuple>
#include <vector>
#include <string>
#include <map>
#include <thread>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

//namespace tmp  // for the moment...
//{
    template<typename T>
    struct is_serializeable_tmp : std::true_type {};

    template<class T, class = void>  struct has_arch_type                                       : std::false_type {};
    template<class T>                struct has_arch_type <T, std::void_t<typename T::arch_t>>  : std::true_type  {};

    template<class ARCH, class T>   struct matching_splitter  : std::false_type {};

    template<class ARCH, class T>
    requires (not is_splitter_v<T>)
    struct matching_splitter<ARCH,T>  : std::true_type {};

    template<class ARCH, class T>
    requires (is_splitter_v<T>)
    struct matching_splitter <ARCH, T> : std::integral_constant<bool, std::is_same_v<ARCH, typename T::arch_t> > {};

//}

template <class ARCH, class TASK, typename ...ARGS>
concept runnable = requires (TASK task, ARGS&&... args)
{
    // TASK must have an operator() method
    task (std::forward<ARGS>(args)...);

    // The arguments must be serializable, ie. they can be put into some buffer as
    // a vehicle between two end points. For instance with DPU, such a buffer will
    // be broadcasted between the host and the DPU
    // NOTE: this constraint is commented -> need to re-design the serialization part
    // and 'is_serializable' will be then properly defined.
    requires ( std::conjunction_v<is_serializeable_tmp<ARGS>...> );

    // In case of a 'split' argument, we should check that the template parameter of 'split'
    // is coherent with the given architecture. For instance, if we have split<ArchUpmem::Tasklet>
    // we must check that the given ARCH is actually ArchUpmem
    //requires (std::conjunction_v <matching_splitter<ARCH,ARGS>... >);
};

////////////////////////////////////////////////////////////////////////////////

/** \brief Class that allows to run some task on a given architecture
 *
 * The Launcher class provides methods for launching a task on a given architecture.
 *
 * Its main responsibility is to separate the logical view of "running" something to
 * the underlying physical architecture provided by the template class ARCH.
 *
 * The end user should use this class for running tasks; by doing so, it becomes
 * possible to run the same task on different architecture by providing a specific
 * ARCH implementation.
 *
 * \param[in] ARCH : class providing resources for a specific hardware architecture
 *
 * \see bpl::ArchUpmem
 * \see bpl::ArchMulticore
 */
template <class ARCH>
class Launcher
{
public:

    using arch_t = ARCH;

    /** Constructor. Perfect forwarding used here in order to initialize the underlying architecture.
     * \param[in] args : parameters for initializing the architecture
     */
    template<typename... ARGS>
    Launcher (ARGS&&... args)  : arch_ (std::forward<ARGS>(args)...)  {}

    /** Return the name of the architecture.
     * \return the architecture name
     */
    std::string name() { return arch_.name(); }

    /** Return the number of process units. In our context, a process unit is a logical feature that executes some task.
     * In a multi-core context for instance, a thread is a process unit. In a UPMEM context, a tasklet is a process unit.
     *
     * This method is useful to know "how much power" this launcher has in order to parallelize some task. The result of this
     * method is delegated to the ARCH attribute.
     *
     * \return the number of process units
     */
    size_t getProcUnitNumber() const { return arch_.getProcUnitNumber(); }

    auto getProcUnitDetails()  const { return arch_.getProcUnitDetails(); }

    /** Run a task on the underlying architecture.
     *
     * Launcher has the responsibility to check whether the end user has provided some information for splitting the input
     * parameters (i.e. information for parallelization of the task). In other words, Launcher knows how to generate several
     * execution contexts and send them to the architecture layer.
     *
     * A task here is defined as a class or struct that provides (1) the name of the task and (2) the method that actually
     * does the required job.
     *
     * \param[in] TASK: class/struct providing the task execution model
     * \param[in] args: input parameters for the task to be executed
     * \return result from the task
     */
    template<template<typename ...> class TASK, typename...TRAITS, typename ...ARGS>
    // WARNING!!! temporarely remove the concept here because of split management when mixing archs
    // (e.g. unit tests on host like 'SplitDifferentSizes')
    //requires runnable<ARCH,TASK<ARCH,TRAITS...>,ARGS...>
    auto  run (ARGS&&... args)
    {
        DEBUG_LAUNCHER ("[Launcher::run] BEGIN\n");

        using task_t = TASK<ARCH,TRAITS...>;

        DEBUG_LAUNCHER ("[Launcher::run] executing\n");

        // We delegate the execution to the architecture instance.
        auto result = arch_.template run<TASK,TRAITS...> (std::forward<ARGS>(args)...);

        // We define a Reduce functor that can (or not) reduce the partial results.
        Reduce<has_reduce<task_t>::value,task_t> reducer;

        DEBUG_LAUNCHER ("[Launcher::run] reducing  %ld items\n", result.size());

        // We return the reduced results.
        auto res = reducer(result);

        DEBUG_LAUNCHER ("[Launcher::run] END\n");

        return res;
    }

    const auto& getStatistics() const
    {
        return arch_.getStatistics();
    }

private:

    /** Object representing the architecture. Most of the Launcher class will delegate the work to this object. */
    ARCH arch_;
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
