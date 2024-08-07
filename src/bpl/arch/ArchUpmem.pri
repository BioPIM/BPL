////////////////////////////////////////////////////////////////////////////////
// BPL, the BioPIM Library  (2023)
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

// NOTE: extern "C" required here since we compile in C++ world.
extern "C"
{
    #include <dpu.h>
    #include <dpu_memory.h>
    #include <dpu_management.h>
}

#include <string>

////////////////////////////////////////////////////////////////////////////////
namespace details {
///////////////////////////////////////////////////////////////////////////////

class dpu_set_handle_t
{
public:
    
    dpu_set_handle_t (DpuComponentKind_e kind=DpuComponentKind_e::RANK, size_t nbpu=1, const char* options=nullptr, bool trace=false)
    {
        if (options==nullptr)  { options = getenv("DPU_OPTIONS"); }
        if (options==nullptr)  { options = ""; }

        options  = "sgXferEnable=true,sgXferMaxBlocksPerDpu=32";
        
        options_ = options!=nullptr ? options : "";

             if (kind==DpuComponentKind_e::RANK) { DPU_ASSERT(dpu_alloc_ranks (nbpu,  options, &handle_));  }
        else if (kind==DpuComponentKind_e::DPU)  { DPU_ASSERT(dpu_alloc       (nbpu,  options, &handle_));  }
        //else                                     { throw std::runtime_error ("bad DPU allocation"); }
             
        // We retrieve the number of DPU for the current set.
        DPU_ASSERT(dpu_get_nr_dpus(handle(),  &nbDPUs_));

        DPU_ASSERT(dpu_get_nr_ranks(handle(), &nbRanks_));
        
        // We compute the number of process units.
        nbProcUnits_ = nbDPUs_ * NR_TASKLETS;
        
        trace_ = trace;
    }

    ~dpu_set_handle_t ()
    {
        dump();
        
        DPU_ASSERT(dpu_free(handle_));
    }

    const std::string& getOptions () const { return options_; }
    
    void dump ()
    {
        if (trace_) 
        {
            struct dpu_set_t dpu;  DPU_FOREACH(handle(), dpu) 
            {  
                // First pass: redirect to blackhole file -> just interested by return error code
                auto err = dpu_log_read(dpu, core::BlackHoleFile::get());
                if (err == DPU_OK)  {  DPU_ASSERT(dpu_log_read(dpu, stdout));  }
            }
        }
    }
    
    size_t getProcUnitNumber() const { return nbProcUnits_; }

    size_t getDpuNumber() const { return nbDPUs_; }

    size_t getRanksNumber() const { return nbRanks_; }

    struct dpu_set_t& handle()  { return handle_; }

private:
    struct dpu_set_t handle_;
    uint32_t nbDPUs_      = 0;
    uint32_t nbRanks_     = 0;
    uint32_t nbProcUnits_ = 0;
    bool     trace_       = false;
    std::string options_;
};

///////////////////////////////////////////////////////////////////////////////
}; // end of namespaces
///////////////////////////////////////////////////////////////////////////////
