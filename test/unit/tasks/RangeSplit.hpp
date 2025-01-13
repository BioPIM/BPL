////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct RangeSplit : bpl::Task<ARCH>
{
    USING(ARCH);

    using type = pair<uint64_t,uint64_t>;

    auto operator() (
        type r1,
        type r2,
        type r3,
        type r4
    )
    {
        return make_tuple (r1, r2, r3, r4);
    }
};
