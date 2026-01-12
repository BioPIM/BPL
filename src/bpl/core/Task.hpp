////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/utils/metaprog.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** Type trait that allows to retrieve the architecture type from a task type.
 */
template <typename T>  struct get_crtp_template;

/** Specialization of get_crtp_template.
 */
template <template <typename,int> class TASK, typename ARCH, int NB>
struct get_crtp_template<TASK<ARCH,NB>>
{
    /** Architecture associated to the task. */
    using arch_t = ARCH;

    /** Number of mutexes associated to the task. */
    static constexpr int MUTEX_NB = NB;
};

////////////////////////////////////////////////////////////////////////////////
/**  Base class for the CRTP.
 * This class factorizes some features for all kinds of tasks.
 * \param DERIVED: type holding the task type.
 */
template<typename DERIVED> struct TaskBase
{
    /** Architecture type of the underlying task. */
    using arch_t  = typename get_crtp_template<DERIVED>::arch_t;

    /** Mutex type of the task. */
    using mutex_t = typename arch_t::mutex;

    /** Number of mutexes available for an instance of the task type. */
    static constexpr int MUTEX_NB = get_crtp_template<DERIVED>::MUTEX_NB;

    using tuid_t   = uint32_t;
    using guid_t   = uint32_t;
    using cycles_t = uint32_t;

    /** Return the task object with its actual type (ie. not the CRTP base class) */
    const DERIVED& self() const { return static_cast<const DERIVED&> (*this); }

    /** Get the process unit id of the task (thread id, tasklet id... according
     * to the underlying architecture)
     * \return the process unit identifier
     */
    auto tuid () const  { return tuid_; }

    /** Get the group of the process unit. In case of UPMEM, it will be the logical identifier of the DPU
     * for the current process unit (ie. the tasklet)
     * \return the group identifier of the process unit.
     */
    auto guid () const  { return guid_; }

    /** Tells whether the provided process unit id belongs to the same group as the process unit
     * of the current Task object.
     * \param value: process unit id to be compared to the current one
     * \return true if the provided process unit belongs to the same group as the current one.
     */
    bool match_tuid (tuid_t value)  const { return self().match_tuid(value); }

    /** Configure some attributes of the task. Should be called after construction of the Task object.
     * \param tuid: the process unit id of the task
     * \param guid: group id of the process unit of the task
     * \param t0: returns a time stamp and should be used as initial timestamp for computing statistics.
     */
    void configure (tuid_t tuid, guid_t guid, cycles_t t0)
    {
        tuid_    = tuid;
        guid_    = guid;
        t0_      = t0;
    }

    /** Get the number of cycles that occured since the t0 time (ie since call to 'configure)
     * \return the number of cycles.
     */
    cycles_t  elapsed () const  { return self().nbcycles() - t0_; }

    /** Get the nth available mutex for the task. Note: nothing is done to check whether the index
     * is valid or not.
     * \param idx: the mutex index.
     * \return a mutex object.
     */
    mutex_t& get_mutex (int idx=0)  {  return  mutexes[idx];   }

    tuid_t   tuid_;
    guid_t   guid_;
    cycles_t t0_;

    template<typename...ARGS>
    void log (ARGS...args) const  { }

    static array_wrapper<mutex_t,MUTEX_NB> mutexes;
};

////////////////////////////////////////////////////////////////////////////////
/** Class that provides runtime information to the end user.
 *
 * When creating a new task, the developer has the possibility to inherit from
 * bpl::Task in order to get some useful information such as the process unit
 * identifier with the Task::tuid method, or to have access to mutexes when
 * required.
 *
 * Note this class is optional; if there is not need to get runtime information,
 * the developer should not inherit from it.
 *
 * \param ARCH: the architecture to be associated to the task
 * \param MUTEX: number of mutexes available to the task.
 */
template<typename ARCH, int MUTEX=1> struct Task : TaskBase<Task<ARCH,MUTEX>>
{
    using parent_t = TaskBase<Task<ARCH,MUTEX>>;
    using tuid_t   = typename parent_t::tuid_t;
    using cycles_t = typename parent_t::cycles_t;
    static constexpr int MUTEX_NB = MUTEX;

    cycles_t nbcycles() const  { return 0;     }

    void notify (size_t current, size_t total) {}
};

/** Type traits telling whether a type T is derived from TaskBase  */
template<typename T>
static constexpr bool is_task_v = is_base_of_template_v<TaskBase,T>;

/** Type traits telling whether a TASK type has a method 'split' for a given type T  */
template<typename TASK,typename T>
static constexpr bool is_custom_splitable_v = requires(const T& t, size_t idx, size_t total ) { TASK::split(t, idx, total); };

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
