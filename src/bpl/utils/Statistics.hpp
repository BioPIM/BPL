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

    TimeStamp produceTimestamp (const char* label, bool cumul=false)  {
        if (not cumul)  { timings[label]={}; }
        return TimeStamp (timings[label]);
    }

    auto produceDualTimestamp (const char* l1, const char* l2, bool c1=false, bool c2=false)  {
        if (not c1)  { timings[l1]={}; }
        if (not c2)  { timings[l2]={}; }
        return DualTimeStamp (TimeStamp (timings[l1]), TimeStamp (timings[l2]));
    }

    auto produceCumulTimestamp (const char* prefix, const char* suffix)  {
        std::string nocumul = std::string(prefix) + "/once/"  + std::string(suffix);
        std::string   cumul = std::string(prefix) + "/cumul/" + std::string(suffix);
        timings[nocumul]={};
        return DualTimeStamp (TimeStamp (timings[nocumul]), TimeStamp (timings[cumul]));
    }

    void dump(bool force=false) const
    {
        char* d = getenv("BPL_LOG");
        if (force or d!=nullptr)
        {
            printf ("[Statistics]  \n");

            printf ("   calls  : %ld\n", callsNb.size());
            for (auto entry : callsNb)
            {
                printf ("       %-35s: %4ld\n", entry.first.c_str(), entry.second);
            }

            printf ("   timings: %ld\n", timings.size());
            for (auto entry : timings)
            {
                printf ("       %-35s: %7.4f\n", entry.first.c_str(), entry.second);
            }

            printf ("   tags   : %ld\n", tags.size());
            for (const auto& entry: tags)
            {
                printf ("       %-35s: %s\n", entry.first.c_str(), entry.second.c_str());
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

    void addTiming (const std::string& key, double value) { timings[key] = value; }

    auto const& getTag (const std::string& key) const { return tags.at(key); }

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
