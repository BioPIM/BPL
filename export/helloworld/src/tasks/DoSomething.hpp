////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
struct MyData
{
    int x;  int y; double z;  char t;
};

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct DoSomething : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (
        const MyData& data,
        const pair<int,int>& coords,
        char c
    )
    {
        // We just return the process unit identifier + the parameters we got as input.
        return make_tuple (this->tuid(), data, coords, c);
    }
};
