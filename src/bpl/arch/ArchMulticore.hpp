////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once
////////////////////////////////////////////////////////////////////////////////

#include <bpl/arch/Arch.hpp>
#include <bpl/arch/ArchMulticoreResources.hpp>
#include <bpl/core/Task.hpp>
#include <bpl/utils/metaprog.hpp>
#include <bpl/utils/std.hpp>  // for 'apply'
#include <bpl/utils/TaskUnit.hpp>
#include <bpl/utils/Statistics.hpp>
#include <bpl/utils/splitter.hpp>
#include <bpl/utils/split.hpp>
#include <bpl/utils/Range.hpp>

#include <vector>
#include <array>
#include <tuple>
#include <string>
#include <cstring>

#include <thread>

#include <BS_thread_pool.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** \brief Class that provides methods and types for a multi-core architecture
 *
 * The main purpose of this class is to provide a 'run' method that will dispatch
 * a task over several threads (according to the object's initialization).
 *
 * This class also provides the number of process units it can manage. Here, a process
 * unit is a thread.
 *
 * It also provides some types that can be used within the execution of the task. By default in
 * this implementation, such types will be mainly std containers from the standard library.
 *
 * \see bpl::Launcher
 */
class ArchMulticore : public ArchMulticoreResources
{
private:

    BS::thread_pool threadpool_;

public:

    // Factory that returns a type with a specific configuration.
    template<typename CFG=void> using factory = ArchMulticore;

    // Shortcut
    using arch_t = ArchMulticore;

    class Thread : public TaskUnit
    {
    public:
        using arch_t = ArchMulticore;
        using TaskUnit::TaskUnit;
        constexpr static int LEVEL = 1;
        std::size_t getNbUnits() const { return getNbComponents()*1; }
    };

    using lowest_level_t = Thread;

    template<typename TASKUNIT=Thread>
    ArchMulticore (TASKUNIT taskunit = TASKUNIT(1),
        [[maybe_unused]] bool trace=false,
        [[maybe_unused]] bool reset=false
    ) : threadpool_(taskunit.getNbComponents())
    {
        nbThreads_ = taskunit.getNbComponents();
    }

    /** Constructor.
     * \param[in] nbThreads : number of usable threads for this architecture.
     */
    ArchMulticore (int nbThreads=1) : nbThreads_ (nbThreads) {}

    /** Return the name of the current architecture.
     * \return the architecture name
     */
    std::string name() { return "multicore"; }

    /** Return the number of threads.
     * \return the number of process units
     */
    size_t getProcUnitNumber() const { return nbThreads_; }

    auto getProcUnitDetails()  const { return std::make_tuple(getProcUnitNumber()); }

    ////////////////////////////////////////////////////////////////////////////////
    /** Configure an object of type T. We distinguish here two cases:
     *   - if T is derived from Task, the we call the 'Task::configure' method
     *   - if T is not derived from Task, there is no configuration.
     */
    template<typename T, typename...ARGS>
    auto configure (T&& task, ARGS...args)  {}

    template<typename T, typename...ARGS>
    requires (bpl::is_task_v<T>)
    auto configure (T&& task, ARGS...args)
    {
        task.configure (args...);
    }

    /** Execute a task for a given parameters pack.
     * For the multi-core architecture, we use a pool of threads.
     * \param[in] args: arguments to be provided to the task.
     */
    template<template<typename ...> class TASK, typename...TRAITS, typename ...ARGS>
    auto run (ARGS&&...args)
    //auto run (const std::vector<std::tuple<ARGS...>>& configurations)
    {
        using task_t = TASK<arch_t,TRAITS...>;

        auto ts = statistics_.produceTimestamp("run/launch");

        // We determine the result type of the TASK::run method.
        // Note that the TASK type is instantiated with the current ARCH type.
        using result_t = bpl::return_t<decltype(&task_t::operator())>;

        // We create the result vector.
        std::vector<result_t> results;

        auto loop_future = threadpool_.submit_sequence <std::size_t> (0, getProcUnitNumber(),  [&] (std::size_t idx)
        {
            task_t task;

            // We may have to configure the task, according to the fact that its class inherits (or not) from bpl:Task
            configure (
                task,
                idx, // process unit uid
                0,   // group uid: only one group for multicore (with idx 0)
                0
            );

            auto config = prepare<ARGS...>(idx,getProcUnitNumber(),std::tuple<ARGS...> {args...});

            // we use 'apply' here to unpack the current tuple in order to feed the 'run' method of the task.
            return bpl::apply ( [&](auto &&... args)  {  return task (args...);  },
                config
            );
        });

        for (auto&& res : loop_future.get()) { results.push_back (res); }

        // We return the partial results as a vector.
        return results;
    }

    /** Transformation of the parameters pack according to the presence or not of a SplitProxy
     *  For each parameter:
     *    - if it is a SplitProxy, we actually split the proxied object
     *    - otherwise, we use the object itself without transformation.
     */
    template<typename ...ARGS>
    auto prepare (size_t idx, size_t nbitems, const std::tuple<ARGS...>& targs)
    {
        return bpl::transform_each (targs, [&] (auto&& x)
        {
            using dtype = std::decay_t<decltype(x)>;

            if constexpr (::details::GetSplitStatus<dtype,lowest_level_t>::value > 0)
            {
                return SplitOperator<dtype>::split (x, idx, nbitems);
            }
            else
            {
                return x;
            }
        });
    }

    /** Get statistics.
     * \return the statistics.
     */
    const Statistics& getStatistics() const { return statistics_; }

private:

    /** Number of threads usable for this architecture. */
    size_t nbThreads_ = 1;

    Statistics statistics_;
};

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////

inline auto operator""_thread (unsigned long long int nb)  {   return bpl::ArchMulticore::Thread{nb};  }


#include <bpl/utils/BufferIterator.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////
template<>
class BufferIterator<ArchMulticore>
{
public:
    using pointer_t = char*;

    BufferIterator (const pointer_t& buf) : buf_(buf) {}

    template<typename T> T& object (std::size_t size) { return  * ((T*)buf_); }

    void memcpy (void* target, std::size_t size)  {  ::memcpy (target, buf_, size); }

    void advance (std::size_t n)   { buf_ += n; }

    pointer_t get() const { return buf_; }

private:
    pointer_t buf_;
};

////////////////////////////////////////////////////////////////////////////////
};
////////////////////////////////////////////////////////////////////////////////
