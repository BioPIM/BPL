////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <firstinclude.hpp>
#include <vector>

#define WITH_STATS_TIME_MEDIAN 1

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// STATISTICS
//
// Some structures for gathering statistics during execution. These statistics
// will be sent to the host and will be available through the calling launcher.
////////////////////////////////////////////////////////////////////////////////

/** \brief Generic class that gathers time information during a tasklet execution.
 */
template<typename T>
struct TimeStatsValues
{
    /** Total time of the tasklet. */
    T all         = T();
    /** Time taken by a tasklet for the unserialization of the incoming arguments from the host. */
    T unserialize = T();
    /** Time taken by a tasklet for splitting the arguments encapsulated by a call to 'split' at host call site. */
    T split       = T();
    /** Time taken by a tasklet for the running the task, ie. call to operator(). */
    T exec        = T();
    /** Time taken by a tasklet for preparing the result for the host. */
    T result      = T();

    /** Increase the fields values of the current object with the fields values of an input object. */
    template<typename T1>
    TimeStatsValues<T>& operator+= (const TimeStatsValues<T1>& o)
    {
        all         += o.all;
        unserialize += o.unserialize;
        split       += o.split;
        exec        += o.exec;
        result      += o.result;
        return *this;
    }
};

/** \brief Class that gathers time information during a tasklet execution.
 *
 * Uses a specific type for gathering the information.
 * \see TimeStatsValues
 */
struct TimeStats : TimeStatsValues<uint64_t>
{
    using value_type = uint64_t;

    static auto minmax (const std::vector<TimeStats>& entries)
    {
        TimeStats m;
        TimeStats M;

        m.all = std::numeric_limits<value_type>::max();
        M.all = 0;

        std::vector<value_type> alls;
        for (const auto& x : entries)
        {
            alls.push_back (x.all);
            if (x.all < m.all)  { m = x; }
            if (x.all > M.all)  { M = x; }
        }

#ifdef WITH_STATS_TIME_MEDIAN
        std::sort(alls.begin(), alls.end());
#endif
        std::array<value_type,32> quantiles = {};

        if (not alls.empty())
        {
            size_t idx=0; for (auto& x : quantiles) { x = alls[idx*alls.size()/quantiles.size()]; idx++; }
            quantiles.back() = alls.back();
        }

        return std::make_tuple (m,M,quantiles);
    }

    static auto mean (const std::vector<TimeStats>& entries)
    {
        TimeStatsValues<uint64_t> res;

        for (const auto& x : entries)  {  res += x;  }

        return TimeStatsValues<double>
        {
            double(res.unserialize) / double(res.all),
            double(res.split)       / double(res.all),
            double(res.exec)        / double(res.all),
            double(res.result)      / double(res.all),
            double(res.all)         / double(entries.size())
        };
    }
};

/** \brief provides information about memory allocations (notably for the MRAM)
 */
struct AllocatorStats
{
    /** Bytes number used in MRAM. */
    uint32_t used        = 0;
    /** Position in MRAM of the last pointer. */
    uint32_t pos         = 0;
    /** Number of calls to the bpl::MRAM::get function. */
    uint32_t nbCallsGet  = 0;
    /** Number of calls to the bpl::MRAM::read function. */
    uint32_t nbCallsRead = 0;
};

////////////////////////////////////////////////////////////////////////////////
// INPUT
////////////////////////////////////////////////////////////////////////////////
/** \brief Structure holding metadata sent from the host to the DPU.
 */
struct MetadataInput
{
    /** Maximum number of parameters allowed in the prototype of a task (should be better defined at another location). */
    static constexpr int ARGS_MAX_NUMBER = 32;

    MetadataInput ()  {  for (size_t i=0; i<ARGS_MAX_NUMBER; i++)  {  argsSplitStatus[i] = 0; }  }

    MetadataInput (uint32_t nbtaskunits, uint32_t dpuid, uint32_t bufferSize, uint32_t deltaOnce, uint32_t reset, uint32_t oncePadding, uint8_t splitStatus[ARGS_MAX_NUMBER])
    :   nbtaskunits(nbtaskunits), dpuid(dpuid), bufferSize(bufferSize), deltaOnce(deltaOnce), reset(reset), oncePadding(oncePadding)
    {
        for (size_t i=0; i<ARGS_MAX_NUMBER; i++)  {  argsSplitStatus[i] = splitStatus[i]; }
    }

    /** Not used. */
    uint32_t nbtaskunits = 0;
    /** Holds a logical DPU identifier. Allows to compute a global tasklet uid. */
    uint32_t dpuid       = 0;
    /** Provides information where the beginning of the MRAM should be available to the user. */
    uint32_t bufferSize  = 0;
    /** Not used. */
    uint32_t deltaOnce   = 0;
    /** Allows to reset the MRAM allocator. */
    uint32_t reset       = 0;
    /** Padding information when the tag 'once' has been used on a parameter of the task. */
    uint32_t oncePadding = 0;
    /** Gives the split status for the arguments of the task. */
    uint8_t  argsSplitStatus[ARGS_MAX_NUMBER];
};

////////////////////////////////////////////////////////////////////////////////
// OUTPUT
////////////////////////////////////////////////////////////////////////////////
/** \brief Structure holding metadata sent from the DPU to the host.
 */
struct MetadataOutput
{
    /** Keeps information for a vector (address and nb items) when it is contiguous in MRAM.
     * With this information, the host can proceed some optimization for the serialization in
     * order to retrieve the vector data.
     */
    struct VectorInfo {
        uint32_t address = 0;
        uint32_t nbitems = 0;
    };

    /** Provides the global tasklet ids, ie something like dpuid*NR_TASKLETS + me(). */
    uint32_t        result_tasklet_order [NR_TASKLETS];
    /** Data size to be broadcasted from DPU to host. */
    uint32_t        result_tasklet_size  [NR_TASKLETS];
    /** Time statistics at different points of a tasklet. */
    TimeStats       nb_cycles            [NR_TASKLETS];
    /** Provides MRAM address information when the task result is a vector. */
    VectorInfo      vector_info          [NR_TASKLETS];
    /** Statistics from the MRAM allocator. */
    AllocatorStats  allocator_stats;
    /** MRAM pointer after task execution. */
    uint32_t        heap_pointer      = 0;
    /** MRAM pointer before task execution. */
    uint32_t        heap_pointer_init = 0;
    /** Number of clocks per second (provided by UPMEM) */
    uint32_t        clocks_per_sec    = 0;
    /** Stack size (corresponds to the constant STACK_SIZE_DEFAULT) */
    uint32_t        stack_size        = 0;
    /** Not used */
    uint32_t        input_sizeof      = 0;
    /** sizeof of the result of the task. */
    uint32_t        output_sizeof     = 0;
    /** Not used */
    uint32_t        restoreNbErrors   = 0;

    /** \brief Provides some statistics for the bpl::vector class (for debug purpose). */
    struct VectorStats
    {
        uint32_t SIZEOF;
        uint32_t NB_VECTORS_IN_PROTO;
    	uint32_t CACHE_NB;
    	uint32_t MEMORY_SIZE;
    	uint32_t CACHE_NB_ITEMS;
        uint64_t NBITEMS_MAX;
        uint32_t MEMTREE_NBITEMS_PER_BLOCK;
        uint32_t MEMTREE_MAX_MEMORY;
        uint32_t MEMTREE_LEVEL_MAX;
    } vstats;
};

////////////////////////////////////////////////////////////////////////////////
};
////////////////////////////////////////////////////////////////////////////////
