////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <firstinclude.hpp>
#include <vector>

#define WITH_STATS_TIME_MEDIAN 1

////////////////////////////////////////////////////////////////////////////////
// STATISTICS
//
// Some structures for gathering statistics during execution. These statistics
// will be sent to the host and will be available through the calling launcher.
////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct TimeStatsValues
{
    T all         = T();
    T unserialize = T();
    T split       = T();
    T exec        = T();
    T result      = T();

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

struct AllocatorStats
{
    uint32_t used        = 0;
    uint32_t pos         = 0;
    uint32_t nbCallsGet  = 0;
    uint32_t nbCallsRead = 0;
};

////////////////////////////////////////////////////////////////////////////////
// INPUT
////////////////////////////////////////////////////////////////////////////////
struct MetadataInput
{
    static constexpr int ARGS_MAX_NUMBER = 32;

    MetadataInput ()  {  for (size_t i=0; i<ARGS_MAX_NUMBER; i++)  {  argsSplitStatus[i] = 0; }  }

    MetadataInput (uint32_t nbtaskunits, uint32_t dpuid, uint32_t bufferSize, uint32_t deltaOnce, uint32_t reset, uint32_t oncePadding, uint8_t splitStatus[ARGS_MAX_NUMBER])
    :   nbtaskunits(nbtaskunits), dpuid(dpuid), bufferSize(bufferSize), deltaOnce(deltaOnce), reset(reset), oncePadding(oncePadding)
    {
        for (size_t i=0; i<ARGS_MAX_NUMBER; i++)  {  argsSplitStatus[i] = splitStatus[i]; }
    }

    uint32_t nbtaskunits = 0;
    uint32_t dpuid       = 0;
    uint32_t bufferSize  = 0;
    uint32_t deltaOnce   = 0;
    uint32_t reset       = 0;
    uint32_t oncePadding = 0;
    uint8_t  argsSplitStatus[ARGS_MAX_NUMBER];
};

////////////////////////////////////////////////////////////////////////////////
// OUTPUT
////////////////////////////////////////////////////////////////////////////////
struct MetadataOutput
{
    struct VectorInfo {
        uint32_t address = 0;
        uint32_t nbitems = 0;
    };

    uint32_t        result_tasklet_order [NR_TASKLETS];
    uint32_t        result_tasklet_size  [NR_TASKLETS];
    TimeStats       nb_cycles            [NR_TASKLETS];
    VectorInfo      vector_info          [NR_TASKLETS];
    AllocatorStats  allocator_stats;
    uint32_t        heap_pointer      = 0;
    uint32_t        heap_pointer_init = 0;
    uint32_t        clocks_per_sec    = 0;
    uint32_t        stack_size        = 0;
    uint32_t        input_sizeof      = 0;
    uint32_t        output_sizeof     = 0;
    uint32_t        restoreNbErrors   = 0;

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
