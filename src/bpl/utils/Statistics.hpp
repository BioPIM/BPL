////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <map>
#include <string>
#include <bpl/utils/TimeUtils.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

class Statistics
{
public:

    TimeStamp produceTimestamp (const char* label)  {  return TimeStamp (timings[label]);  }

    void dump(bool force=false) const
    {
        char* d = getenv("BPL_LOG");
        if (force or d!=nullptr)
        {
            printf ("[Statistics]  \n");

            printf ("   calls  : %ld\n", callsNb.size());
            for (auto entry : callsNb)
            {
                printf ("       %-27s: %4ld\n", entry.first.c_str(), entry.second);
            }

            printf ("   timings: %ld\n", timings.size());
            for (auto entry : timings)
            {
                printf ("       %-27s: %7.4f\n", entry.first.c_str(), entry.second);
            }

            printf ("   tags   : %ld\n", tags.size());
            for (const auto& entry: tags)
            {
                printf ("       %-27s: %s\n", entry.first.c_str(), entry.second.c_str());
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
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
