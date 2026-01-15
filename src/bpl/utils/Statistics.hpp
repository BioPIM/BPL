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

/** \brief Utility for gathering statistics information.
 *
 * This class provides an API for retrieving statistic information such as duration
 * or tracing calls number for some functions. This is useful for debugging/benchmarking
 * the library itself.
 */
class Statistics
{
public:

    /** Produce a TimeStamp object for a given label.
     * \param label: the label associated to the timestamp
     * \param cumul: if true and if there is already a timing for the label, the same timing
     * is reused, so we will accumulate the durations. If true, we reset the timing if already
     * used before
     * \return a TimeStamp object associated to the input label.
     */
    TimeStamp produceTimestamp (const char* label, bool cumul=false)  {
        if (not cumul)  { timings[label]={}; }
        return TimeStamp (timings[label]);
    }

    /** Produce two TimeStamp objects for a given labels.
     * \param l1: the label associated to the first timestamp
     * \param l2: the label associated to the second timestamp
     * \param c1: cumul status for the first label
     * \param c2: cumul status for the second label
     * \return a DualTimeStamp object.
     */
    auto produceDualTimestamp (const char* l1, const char* l2, bool c1=false, bool c2=false)  {
        if (not c1)  { timings[l1]={}; }
        if (not c2)  { timings[l2]={}; }
        return DualTimeStamp (TimeStamp (timings[l1]), TimeStamp (timings[l2]));
    }

    /** Produce two TimeStamp objects for a given labels with cumul and non cumul.
     * \param prefix: prefix added to the label
     * \param suffix: the actual label
     */
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

    /** Return the map of timings. */
    const std::map<std::string, float>&  getTimings () const { return timings; }

    /** Return the map of calls number. */
    const std::map<std::string, size_t>& getCallsNb () const { return callsNb; }

    /** Return the timing for a given key. */
    float getTiming(const std::string& key) const
    {
        auto lookup = timings.find (key);
        return lookup != timings.end() ? lookup->second : 0.0;
    }

    /** Increment the calls number for a given key. */
    void increment (const std::string& key)
    {
        callsNb[key] ++;
    }

    /** Set the calls number for a given key. */
    void set (const std::string& key, size_t nb)
    {
        callsNb[key] = nb;
    }

    /** Register a key/value. */
    void addTag (const std::string& key, const std::string& value) { tags[key] = value; }

    /** Set the timing for a given key. */
    void addTiming (const std::string& key, double value) { timings[key] = value; }

    /** Get the value of a given key. . */
    auto const& getTag (const std::string& key) const { return tags.at(key); }

    /** Get the calls number of a given key. */
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
