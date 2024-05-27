////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_TIME_UTILS__
#define __BPL_TIME_UTILS__

#include <chrono>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

static inline auto  timestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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
            ref_ += (t1_ - t0_) / 1000.0;
        }
    }

private:
    float& ref_;
    uint64_t  t0_ = 0;
    uint64_t  t1_ = 0;
    bool started_ = false;
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif // __BPL_TIME_UTILS__
