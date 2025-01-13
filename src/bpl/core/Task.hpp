////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/utils/metaprog.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

template <typename T>  struct get_crtp_template;

template <template <typename,int> class TASK, typename ARCH, int NB>
struct get_crtp_template<TASK<ARCH,NB>>
{
    using arch_t = ARCH;
    static constexpr int MUTEX_NB = NB;
};

////////////////////////////////////////////////////////////////////////////////
// Base class for the CRTP
template<typename DERIVED> struct TaskBase
{
    using arch_t  = typename get_crtp_template<DERIVED>::arch_t;
    using mutex_t = typename arch_t::mutex;

    static constexpr int MUTEX_NB = get_crtp_template<DERIVED>::MUTEX_NB;

    using tuid_t   = uint32_t;
    using guid_t   = uint32_t;
    using cycles_t = uint32_t;

    const DERIVED& self() const { return static_cast<const DERIVED&> (*this); }

    auto tuid () const  { return tuid_; }
    auto guid () const  { return guid_; }

    bool match_tuid (tuid_t value)  const { return self().match_tuid(value); }

    void configure (tuid_t tuid, guid_t guid, cycles_t t0)
    {
        tuid_    = tuid;
        guid_    = guid;
        t0_      = t0;
    }

    cycles_t  elapsed () const  { return self().nbcycles() - t0_; }

    tuid_t   tuid_;
    guid_t   guid_;
    cycles_t t0_;

    template<typename...ARGS>
    void log (ARGS...args) const  { }

    static array_wrapper<mutex_t,MUTEX_NB> mutexes;

    mutex_t& get_mutex (int idx=0)  {  return  mutexes[idx];   }
};

//template<typename T>
//typename TaskBase<T>::mutex_t TaskBase<T>::mutexes = {};

////////////////////////////////////////////////////////////////////////////////
template<typename ARCH, int MUTEX=1> struct Task : TaskBase<Task<ARCH,MUTEX>>
{
    using parent_t = TaskBase<Task<ARCH,MUTEX>>;
    using tuid_t   = typename parent_t::tuid_t;
    using cycles_t = typename parent_t::cycles_t;
    static constexpr int MUTEX_NB = MUTEX;

    cycles_t nbcycles() const  { return 0;     }

    void notify (size_t current, size_t total) {}
};

template<typename T>
static constexpr bool is_task_v = is_base_of_template_v<TaskBase,T>;

template<typename TASK,typename T>
static constexpr bool is_custom_splitable_v = requires(const T& t, size_t idx, size_t total ) { TASK::split(t, idx, total); };

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
