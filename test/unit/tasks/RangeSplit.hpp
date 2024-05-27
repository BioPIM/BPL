////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <bpl/core/Task.hpp>

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct RangeSplit : bpl::core::Task<ARCH>
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
////if (this->tuid()==0)
//{
//    printf ("%5d [%ld %ld]  [%ld %ld]  [%ld %ld]  [%ld %ld]   SP\n",
//        this->tuid(),
//        r1.first, r1.second,
//        r2.first, r2.second,
//        r3.first, r3.second,
//        r4.first, r4.second
//    );
//}
        return make_tuple (r1, r2, r3, r4);
    }
};
