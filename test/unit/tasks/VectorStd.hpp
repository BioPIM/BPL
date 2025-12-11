////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <bpl/core/Task.hpp>
#include <vector>

#ifdef DPU
    __mram_noinit  uint8_t __allocator__[1024];
#endif

namespace std
{
   void __throw_length_error(const char*) {}
}

////////////////////////////////////////////////////////////////////////////////

template <class T>
struct Mallocator
{
    typedef T value_type;

    Mallocator() = default;

    template <class U>
    constexpr Mallocator(const Mallocator<U>&) noexcept {}

    T* allocate(std::size_t n)
    {
#ifdef DPU
        return (T*)__MRAM_Allocator_lock__.get (n);
#endif
    }

    void deallocate (T* p, std::size_t n) noexcept {}
};

////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct MyVector : std::vector<T,Mallocator<T>>
{
//    MyVector() : std::vector<T,Mallocator<T>>() {}
};

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorStd: bpl::Task<ARCH>
{
    uint32_t operator() (uint32_t n) const
    {
        MyVector<uint32_t> v;

        for (uint32_t i=1; i<=n; ++i)  {  v.push_back (i);  }

        return v.size();
    }
};
