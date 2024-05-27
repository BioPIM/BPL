////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////

struct Fibonacci
{
    static constexpr uint32_t values[] = { 1,1,2,3,5,8,13,21,34,55,89,144,233,377,610,987,1597,2584,4181,6765,10946,17711,28657,46368,75025,121393,196418,317811,514229,832040,1346269,2178309 };
};

struct EvenInts
{
    static constexpr uint32_t values[] = { 0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,98,100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,136,138,140,142,144,146,148,150,152,154,156,158,160,162,164,166,168,170,172,174,176,178,180,182,184,186,188,190,192,194,196,198,200,202,204,206,208,210,212,214,216,218,220,222,224,226,228,230,232,234,236,238,240,242,244,246,248,250,252,254,256,258,260,262,264,266,268,270,272,274,276,278,280,282,284,286,288,290,292,294,296,298,300,302,304,306,308,310,312,314,316,318,320,322,324,326,328,330,332,334,336,338,340,342,344,346,348,350,352,354,356,358,360,362,364,366,368,370,372,374,376,378,380,382,384,386,388,390,392,394,396,398,400,402,404,406,408,410,412,414,416,418,420,422,424,426,428,430,432,434,436,438,440,442,444,446,448,450,452,454,456,458,460,462,464,466,468,470,472,474,476,478,480,482,484,486,488,490,492,494,496,498,500,502,504,506,508,510,512};
};


////////////////////////////////////////////////////////////////////////////////
template<typename SEQ>
struct IntegerSequence
{
    using value_t = uint32_t;

    std::pair<int,int> range_;

    struct iterator
    {
        iterator (int n) : n_(n) {}

        bool operator!=(const iterator& other)  { return this->n_ != other.n_; }
        value_t operator*() const { return SEQ::values[n_]; }
        iterator& operator++() { ++n_; return *this; }

        int n_;
    };

    std::size_t size() const { return range_.second-range_.first+1; }

    IntegerSequence (std::pair<int,int> range = {0,sizeof(SEQ::values)/sizeof(SEQ::values[0])}) : range_(range) {}

    auto begin() const { return iterator(range_.first);      }
    auto end  () const { return iterator(range_.second);     }
};

////////////////////////////////////////////////////////////////////////////////
template<typename SEQUENCE>
IntegerSequence<SEQUENCE> split (const IntegerSequence<SEQUENCE>& t, std::size_t idx, std::size_t total)
{
    return  IntegerSequence<SEQUENCE> (split(t.range_, idx, total));
}

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct IterableSplit
{
    USING(ARCH);

    auto operator() (const IntegerSequence<EvenInts>& ints)
    {
        uint64_t checksum = 0;
        for (auto n : ints)
        {
            checksum += n;
            //printf ("[%2d]  %d \n", puid, n);
        }

        return checksum;
    }
};
