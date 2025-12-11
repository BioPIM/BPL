////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <chrono>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

static inline auto  timestamp()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

////////////////////////////////////////////////////////////////////////////////
class TimeStamp
{
public:

    TimeStamp(float& ref) : ref_(ref)  {  start();  }

    ~TimeStamp()  { stop();  }

    void start ()
    {
        started_ = true;
        t0_ = t1_ = timestamp();
    }

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
