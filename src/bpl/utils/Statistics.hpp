////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_UTILS_STATISTICS__
#define __BPL_UTILS_STATISTICS__

#include <map>
#include <string>
#include <bpl/utils/TimeUtils.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

class Statistics
{
public:

    core::TimeStamp produceTimestamp (const char* label)  {  return core::TimeStamp (timings[label]);  }

    void dump(bool force=false) const
    {
        char* d = getenv("BPL_LOG");
        if (force or d!=nullptr)
        {
            printf ("[Statistics]  \n");

            printf ("   calls  : %ld\n", callsNb.size());
            for (auto entry : callsNb)
            {
                printf ("       %-25s: %4ld\n", entry.first.c_str(), entry.second);
            }

            printf ("   timings: %ld\n", timings.size());
            for (auto entry : timings)
            {
                printf ("       %-25s: %7.4f\n", entry.first.c_str(), entry.second);
            }

            printf ("   tags   : %ld\n", tags.size());
            for (const auto& entry: tags)
            {
                printf ("       %-25s: %s\n", entry.first.c_str(), entry.second.c_str());
            }

        }
    }

    const std::map<std::string, float>&  getTimings () const { return timings; }
    const std::map<std::string, size_t>& getCallsNb () const { return callsNb; }

    float getTiming(const std::string& key) const
    {
        auto lookup = timings.find (key);
        return lookup != timings.end() ? lookup->second : 0.0;
    }

    void increment (const std::string& key)
    {
        callsNb[key] ++;
    }

    void set (const std::string& key, size_t nb)
    {
        callsNb[key] = nb;
    }

    void addTag (const std::string& key, const std::string& value) { tags[key] = value; }

    size_t getCallNb (const std::string& key) const
    {
        auto lookup = callsNb.find (key);
        return lookup != callsNb.end() ? lookup->second : 0;
    }

private:

    std::map<std::string, std::string>  tags;
    std::map<std::string, float>  timings;
    std::map<std::string, size_t> callsNb;
};

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

struct TimeStats : TimeStatsValues<uint32_t>
{
    static auto minmax (const std::vector<TimeStats>& entries)
    {
        TimeStats m;
        TimeStats M;

        m.all = std::numeric_limits<uint32_t>::max();
        M.all = 0;

        for (const auto& x : entries)
        {
            if (x.all < m.all)  { m = x; }
            if (x.all > M.all)  { M = x; }
        }

        return std::make_tuple (m,M);
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

////////////////////////////////////////////////////////////////////////////////
struct AllocatorStats
{
    uint32_t used        = 0;
    uint32_t nbCallsGet  = 0;
    uint32_t nbCallsRead = 0;
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif // __BPL_CORE_STATISTICS__
