////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <bpl/core/Task.hpp>

template<class ARCH>
struct HelloWorld  : bpl::Task<ARCH>
{
    // We use the 'USING' macro for accessing to some resources associated to the architecture.
    // In this snippet, it allows to access to the 'array' type without prefixing by the namespace.
    USING(ARCH);

    // We define the function that process the task.
    auto operator() (array<char,16> const& data)
    {
        // We compute the checksum of the incoming array (only iteration allowed, i.e. no random access is currently possible).
        uint32_t checksum=0;  for (char c : data)  { checksum += c; }

        // We dump some information.
        printf ("[puid %4ld] got an array of size %ld with checksum %ld \n",
            uint64_t (this->tuid()),
            uint64_t (data.size()),
            uint64_t (checksum)
        );

        // We return some computation.
        return checksum;
    }
};
