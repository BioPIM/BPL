////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#define COUNT_INSTANCES

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct CountInstances
{
    USING(ARCH);

    struct Info
    {
        static constexpr bool parseable = false;

        char dummy = 0;

        static size_t& counts (size_t n)
        {
            static std::size_t arr[] = {0,0,0,0,0,0,0};
            return arr[n];
        }

        ~Info ()                        { counts(0)++; }
        Info ()                         { counts(1)++; }
        Info (char d)                   { counts(2)++; }
        Info (const Info& i)            { counts(3)++; }
        Info (Info&& i)                 { counts(4)++; }
        Info& operator= (const Info& i) { counts(5)++; return *this;}
        Info& operator= (Info&& i)      { counts(6)++; return *this;}

        static void dump (const char* msg="")
        {
            printf ("--------------- %s ------------------ \n", msg);
            printf ("constructor (default): %ld\n", counts(1));
            printf ("constructor (data)   : %ld\n", counts(2));
            printf ("constructor (copy)   : %ld\n", counts(3));
            printf ("constructor (move)   : %ld\n", counts(4));
            printf ("assignment  (copy)   : %ld\n", counts(5));
            printf ("assignment  (move)   : %ld\n", counts(6));
            printf ("destructor           : %ld\n", counts(0));
        }
    };

    auto operator() (const Info& info)
    {
        return 0;
    }
};
