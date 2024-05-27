////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

template<class ARCH>
struct Checksum
{
    USING(ARCH);

    uint64_t operator() (const vector<uint32_t>& v)
    {
        uint64_t checksum = 0;

        for (auto x : v)   {  checksum += x;  }

        return checksum;
    }

    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};
