////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma GCC diagnostic ignored "-Wformat"

#include <bpl/core/Task.hpp>

template<class ARCH>
struct HelloWorld : bpl::core::Task<ARCH>
{
    USING(ARCH);

    auto operator() (const array<char,16>& data)
    {
        int checksum=0;  for (char c : data)  { checksum += c; }

        printf ("[puid %4d] got an array of size %d with checksum %d \n", this->tuid(), data.size(), checksum);

        return checksum;
    }
};
