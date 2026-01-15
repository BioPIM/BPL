////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

template<class ARCH>
struct Checksum
{
    USING(ARCH);

    auto operator() (vector<uint32_t> const& v)   {
        return accumulate(v.begin(), v.end(), uint64_t{0});
    }

    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};
