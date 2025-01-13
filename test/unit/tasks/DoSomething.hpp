////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <bpl/core/Task.hpp>

template<class ARCH>
struct DoSomething : bpl::Task<ARCH>
{
    USING(ARCH);

    auto operator() (const array<char,8>& data, int n)
    {
        printf ("puid: %d\n", this->tuid());
        printf ("n   : %d\n", n);
        printf ("data: ");
        for (char c : data)  { printf ("%c", c); }  printf ("\n");

        return 0;
    }
};
