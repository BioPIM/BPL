////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/bank/Sequence.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

template<int SEQSIZE=16>
class RandomSequenceGenerator
{
public:

    static constexpr int SIZE = SEQSIZE;

    RandomSequenceGenerator()  { this->operator++(); }

    Sequence<SEQSIZE>& operator++ ()
    {
        for (std::size_t i=0; i<Sequence<SEQSIZE>::SIZE; i++)
        {
            seq_.data [i] = Sequence<SEQSIZE>::nucleotids[
                random() % sizeof(Sequence<SEQSIZE>::nucleotids)/sizeof(Sequence<SEQSIZE>::nucleotids[0])
            ];
        }

        return seq_;
    }

    const Sequence<SEQSIZE>& operator*() const { return seq_; }

private:

    Sequence<SEQSIZE> seq_;

    uint32_t random()
    {
        uint32_t a = 16807;
        uint32_t m = 2147483647;
        return seed = (a * seed) % m;
    }

    uint32_t seed = 123456;
};


////////////////////////////////////////////////////////////////////////////////
template<class ARCH, int SEQNB=1024*16, int SEQSIZE=16>
class BankRandom
{
private:
public:
    struct iterator;

public:

    using const_iterator = const iterator;

    BankRandom() {}

    iterator begin() const { return iterator (0,     *this);  }
    iterator end  () const { return iterator (SEQNB, *this);  }

    template<typename FUNCTOR>
    void iterate (FUNCTOR fct)
    {
        std::size_t i=0;
        for (auto&& seq : *this)  {  fct (i++, seq);  }
    }

private:
public:

    struct iterator
    {
        iterator (std::size_t sequencesNumber, const BankRandom& ref)
            : idx_(sequencesNumber),  ref_(ref)
            {  ++generator_; }

        bool operator!=(const iterator& other)  { return this->idx_ != other.idx_; }
        const Sequence<SEQSIZE>& operator*() const { return *generator_; }
        iterator& operator++() {  ++generator_;; ++idx_; return *this; }

        std::size_t        idx_;
        const  BankRandom& ref_;

        RandomSequenceGenerator<SEQSIZE> generator_;
    };

    friend struct iterator;
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
