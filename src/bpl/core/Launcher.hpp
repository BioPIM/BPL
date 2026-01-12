////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
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
#include <any>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
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

    /** Architecture type. This is the type provided as template argument. */
    using arch_t = ARCH;

    /** Create a configuration from variadic arguments.
     * The actual job is delegated to the underlying architecture.
     * \return a std::any object representing the configuration
     */
    template<typename... ARGS>
    static std::any make_configuration (ARGS&&... args)  {
        return arch_t::make_configuration (std::forward<ARGS>(args)...);
    }

    /** Create a Launcher instance from a provide configuration
     * \param cfg: the configuration to be used for creating the launcher (provided as a std::any object)
     * \return a std::unique_ptr object holding the created launcher.
     */
    static auto create (std::any cfg)  {  return std::make_unique <Launcher<ARCH>> (cfg);  }

    /** Constructor from a configuration object provided as a std::any object.
     * \param cfg: the configuration
      */
    Launcher(std::any cfg)  : arch_(cfg) {}

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

    /** Provides more details than getProcUnitNumber. For instance with UPMEM, it will return a std::tuple
     * with the ranks,DPU and tasklets number
     * \return a std::tuple holding detailed information on the process units of the launcher.
     */
    auto getProcUnitDetails()  const { return arch_.getProcUnitDetails(); }

    /** The task unit object used at creation. Delegate to the underlying architecture.
     * \return an instance (as a smart pointer) of the TaskUnit class (dynamic polymorphism here).
     */
    auto getTaskUnit() const { return arch_.getTaskUnit(); }

    /** Run a task on the underlying architecture.
     *
     * Launcher has the responsibility to check whether the end user has provided some information for splitting the input
     * parameters (i.e. information for parallelization of the task). In other words, Launcher knows how to generate several
     * execution contexts and send them to the architecture layer.
     *
     * A task here is defined as a class or struct that provides method that actually
     * does the required job as an overload of operator()
     *
     * The result depends on the task struct: if it contains a specific 'reduce' static function, the result
     * will be reduced in a single object. Otherwise, an iterable object is returned, allowing to iterate
     * each result produced by each process unit.
     *
     * \param[in] TASK: class/struct providing the task execution model
     * \param[in] TRAITS: potential extra type information
     * \param[in] args: input parameters for the task to be executed
     * \return result from the task, either an iterable over all the results or a reduced unique object.
     */
    template<template<typename ...> class TASK, typename...TRAITS, typename ...ARGS>
    auto  run (ARGS&&... args)
    {
        DEBUG_LAUNCHER ("[Launcher::run] BEGIN\n");

        // Type of the actual task that will be used for running the required work.
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

    /** Statistics about the launcher aggregated during execution of tasks through the 'run' method.
     * Delegated to the underlying architecture. Note that theses statistics are not reset between
     * 'run' calls.
     * \return the bpl::Statistic object
     */
    const auto& getStatistics() const
    {
        return arch_.getStatistics();
    }

    /** For testing, it might be interesting to reset the statistics between two calls to 'run'.
     * Delegated to the underlying architecture.
     */
    auto resetStatistics() {
        arch_.resetStatistics();
    }

private:

    /** Object representing the architecture. Most of the Launcher class will delegate the work to this object. */
    ARCH arch_;
};

////////////////////////////////////////////////////////////////////////////////

// Historic stuf, not used in real life code anymore (still in some unit tests though)
template<class ARCH, class T>   struct matching_splitter  : std::false_type {};

template<class ARCH, class T>
requires (not is_splitter_v<T>)
struct matching_splitter<ARCH,T>  : std::true_type {};

template<class ARCH, class T>
requires (is_splitter_v<T>)
struct matching_splitter <ARCH, T> : std::integral_constant<bool, std::is_same_v<ARCH, typename T::arch_t> > {};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
