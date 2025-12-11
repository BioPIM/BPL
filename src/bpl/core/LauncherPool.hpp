////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <memory>
#include <mutex>
#include <any>
#include <bpl/core/Launcher.hpp>
#include <BS_thread_pool.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template <class ARCH>
class LauncherPool
{
public:

    using arch_t     = ARCH;
    using launcher_t = Launcher<ARCH>;

    template<typename TUNIT, typename...ARGS>
    LauncherPool (size_t nbLaunchers, TUNIT units, ARGS&&... args)
        : nbunits_(nbLaunchers * units.getNbComponents())
    {
        threadpool_ = std::make_unique<BS::thread_pool<>> (nbLaunchers);

        // We get a snapshot of the configuration.
        // It will be user later when an actual launcher will be requested for the first time.
        config_ = launcher_t::make_configuration (units, std::forward<ARGS>(args)...);

        launchers_.resize (nbLaunchers);
    }

    size_t getNbComponents  () const  { return nbunits_;     }

    template<template<typename ...> class Task, typename Callback, typename...Args>
    auto submit (Callback cbk, Args&&...args)
    {
        // We need to be sure that the arguments to be used for task execution will have a life cycle long enough.
        // This is important for instance when we directly use 'split' on client side, which implies that this argument
        // is deleted as soon as the call to 'submit' is done. So we make sure here to keep the information in a 'Command'
        // function object that will be used in the 'detach_task' call.
        struct Command
        {
            LauncherPool&       parent;
            Callback            cbk;
            std::tuple<std::decay_t<Args>...> args;

            auto operator() ()
            {
                const std::optional<std::size_t> idx = BS::this_thread::get_index();

                if (idx)
                {
                    // We retrieve the launcher matching the current index
                    launcher_t& launcher = parent.getLauncher(*idx);

                    auto&& results = std::apply ([&] (auto&&... theargs)
                    {
                        // We launch the task and return its result.
                        return launcher.template run<Task> (std::forward<decltype(theargs)>(theargs)...);

                    }, args);

                    // We call the callback with the results
                    cbk (launcher, std::move(results));
                }
                else
                {
                    throw std::runtime_error ("thread pool failed to process the task");
                }
            }
        };
        threadpool_->detach_task ( Command { *this, cbk, std::make_tuple(args...) } );
    }

    auto wait()
    {
        threadpool_->wait();
    }

    size_t size() const { return launchers_.size(); }

private:

    std::any config_;

    size_t nbunits_     = 0;
    size_t nbProcUnits_ = 0;
    std::vector<std::unique_ptr<launcher_t>> launchers_;

    launcher_t& getLauncher (size_t idx) {
        std::lock_guard<std::mutex> g (mutex_);
        // We might create the launcher if not existing
        if (launchers_[idx].get()==nullptr) {
            launchers_[idx] = launcher_t::create (config_);   }
        return *launchers_[idx];
    }

    std::mutex mutex_;

    std::unique_ptr<BS::thread_pool<>> threadpool_;
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
