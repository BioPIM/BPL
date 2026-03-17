////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <chrono>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** Returns a timestamp (in microseconds) of the current time.
 */
static inline auto  timestamp()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

////////////////////////////////////////////////////////////////////////////////
/** \brief Utility for computing duration between two execution points.
 *
 * The variable that holds the duration is given as a reference to the constructor; a timestamp t0 is then generated.
 *
 * When the destructor is invoked, a timestamp t1 is generated and the referred variable is set to be t1-t0.
 *
 * It remains possible to explicitely call 'start' and 'stop'.
 */
class TimeStamp
{
public:

    /** Constructor.
     * \param ref: a reference on the variable that will hold the duration.
     */
    TimeStamp(float& ref) : ref_(ref)  {  start();  }

    /** Destructor.  The final timestamp is generated and the duration computed. */
    ~TimeStamp()  { stop();  }

    /** Generates the initial timestamp. */
    void start ()
    {
        started_ = true;
        t0_ = t1_ = timestamp();
    }

    /** Generates the final timestamp. */
    void stop  ()
    {
        if (started_)
        {
            started_ = false;
            t1_ = timestamp() ;
            ref_ += (t1_ - t0_) / 1000.0 / 1000.0;
        }
    }

private:
    float& ref_;
    uint64_t  t0_ = 0;
    uint64_t  t1_ = 0;
    bool started_ = false;
};

////////////////////////////////////////////////////////////////////////////////

/** \brief Manages two TimeStamp objects at the same time.
 * \see TimeStamp.
 */
class DualTimeStamp
{
public:
    DualTimeStamp(TimeStamp const& t1 , TimeStamp const& t2) : ts(t1,t2)  {}

    void start () {  ts.first.start();  ts.second.start();  }
    void stop  () {  ts.first.stop ();  ts.second.stop ();  }

private:
    std::pair<TimeStamp,TimeStamp> ts;
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
