 

#ifdef DPU
#include <mram.h>
#include <stdio.h>
#include <defs.h>
#define DMA_ALIGNED __dma_aligned
#else
#include <cstddef>
#include <cstdint>
#include <cstdio>
#define DMA_ALIGNED
#endif

#include <vector>
#include <array>
#include <map>

#define DBG

#ifdef DBG
    #define DEBUG(args...)  printf (args)
#else
    #define DEBUG(args...)
#endif

namespace std
{
    void __throw_length_error(char const*) { }
}

////////////////////////////////////////////////////////////////////////////////
static const size_t DPU_BUFFER_SIZE = 1UL<<20;

#ifdef DPU
__mram_noinit
#endif
uint8_t MRAM_buffer [DPU_BUFFER_SIZE];

uint8_t WRAM_buffer[1024];

//////////////////////////////////////////////////////////////////////////////////

#ifdef DPU
template<typename T>  auto mram2wram (T* src, T* tgt)
{
    mram_read ((__mram_ptr void*)src, tgt, sizeof(T));
    return tgt;
}

template<typename T>  auto wram2mram (T* src, T* tgt)
{
    mram_write (src, (__mram_ptr void*)tgt, sizeof(T));
    return tgt;
}
#else
template<typename T>  auto mram2wram (T* src, T* tgt)
{
    *tgt = *src;
    return tgt;
}

template<typename T>  auto wram2mram (T* src, T* tgt)
{
    *tgt = *src;
    return tgt;
}
#endif

template<typename T, int N=16>
auto dumpWRAM()
{
#ifdef DBG
    for (int i=0; i<N*sizeof(T); i+=sizeof(T))  {  uint8_t* ptr = MRAM_buffer+i;  printf ("[0x%6lx] %ld\n", ptr, *ptr);  }
#endif
}

////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct MramPtr
{
    DMA_ALIGNED         T* mramPtr = nullptr;
    DMA_ALIGNED mutable T* wramPtr = nullptr;
    mutable size_t wramIdx = 0;

    auto nextWramIdx() const  {
        wramIdx = (wramIdx+1)%2;
        DEBUG ("[nextWramIdx] his: %p   %ld \n", this, wramIdx);
        return wramIdx; }
    auto getWramIdx () const  { return wramIdx; }

    MramPtr(T *ptrMRAM, T *ptrWRAM, bool) : mramPtr(ptrMRAM), wramPtr(ptrWRAM)
    {
        mram2wram (mramPtr, wramPtr+getWramIdx());
        DEBUG ("[construct]  this: %p  mram: 0x%04lx  wram: 0x%04lx => %ld\n", this, uint64_t(mramPtr), uint64_t(wramPtr), uint64_t(*wramPtr));
    }

    using element_type      = T;
    using difference_type   = std::ptrdiff_t;
    using value_type        = element_type;
    using pointer           = element_type*;
    using reference         = element_type&;
    using iterator_category = std::random_access_iterator_tag;

    MramPtr()               = default;
    MramPtr(const MramPtr&) = default;

    MramPtr& operator=(const MramPtr&) = default;

    ~MramPtr()  { DEBUG ("[destruct]   this: %p \n", this); }

    reference operator*() const
    {
        auto idx = nextWramIdx();

        // We synchronize the current object pointed in MRAM with the WRAM
        auto ptr = mram2wram (mramPtr, wramPtr+idx);
        DEBUG ("[operator*]  this: %p  mram: 0x%04lx  wram: 0x%04lx  idx: %2ld => %ld\n",
            this, uint64_t(mramPtr), uint64_t(wramPtr), idx, uint64_t(*wramPtr)
        );

        return *ptr;
    }

    auto operator->() const
    {
        struct transient
        {
            using element_type = T;
            const MramPtr<T> obj;

             auto operator->() const
             {
                 DEBUG ("[transient::operator->]:  mram: %ld  wram: %ld \n", *obj.mramPtr, *obj.wramPtr);
                 return obj.wramPtr;
             }

            ~transient()
            {
                DEBUG ("[transient::~transient]:  mram: %ld  wram: %ld \n", *obj.mramPtr, *obj.wramPtr);
                wram2mram (obj.wramPtr+obj.getWramIdx(), obj.mramPtr);
            }
        };

        mram2wram (mramPtr, wramPtr+nextWramIdx());

        DEBUG ("[operator->]  %p \n", mramPtr);
        return transient {*this} ;
    }

    explicit operator bool() const { return mramPtr != nullptr; }

    friend bool operator==(MramPtr l, MramPtr r) {  return l.mramPtr == r.mramPtr;  }
    friend bool operator!=(MramPtr l, MramPtr r) {  return !(l == r);       }

    MramPtr& operator++()
    {
        DEBUG ("[operator++ (A)]  this: %p  mram: 0x%04lx  wram: 0x%04lx => %ld\n", this, uint64_t(mramPtr), uint64_t(wramPtr), uint64_t(*wramPtr));
        wram2mram (  wramPtr+getWramIdx(), mramPtr);
//        mram2wram (++mramPtr, wramPtr);
        ++mramPtr;
        DEBUG ("[operator++ (B)]  this: %p  mram: 0x%04lx  wram: 0x%04lx => %ld\n", this, uint64_t(mramPtr), uint64_t(wramPtr), uint64_t(*wramPtr));

        return *this;
    }

    MramPtr& operator--()
    {
        DEBUG ("[operator-- (A)]  this: %p  mram: 0x%04lx  wram: 0x%04lx => %ld\n", this, uint64_t(mramPtr), uint64_t(wramPtr), uint64_t(*wramPtr));
//        wram2mram (  wramPtr, mramPtr);
//        mram2wram (--mramPtr, wramPtr);
        --mramPtr;
        DEBUG ("[operator-- (B)]  this: %p  mram: 0x%04lx  wram: 0x%04lx => %ld\n", this, uint64_t(mramPtr), uint64_t(wramPtr), uint64_t(*wramPtr));

        return *this;
    }

    MramPtr operator++(int) {
        DEBUG ("[operator++(int)] \n");
        return MramPtr(mramPtr++, true);
    }
    MramPtr operator--(int) {
        DEBUG ("[operator--(int)] \n");
        return MramPtr(mramPtr--, true);
    }

    MramPtr& operator+=(difference_type n) {  mramPtr += n;  return *this;  }
    MramPtr& operator-=(difference_type n) {  mramPtr -= n;  return *this;  }

    friend MramPtr operator+(MramPtr p, difference_type n) {  return p += n;  }
    friend MramPtr operator+(difference_type n, MramPtr p) {  return p += n;  }
    friend MramPtr operator-(MramPtr p, difference_type n) {  return p -= n;  }

    friend difference_type operator-(MramPtr const& a, MramPtr const& b)
    {
        DEBUG ("[operator-]  %d  %d  -> %d  \n", a.mramPtr, b.mramPtr, a.mramPtr - b.mramPtr);
        return a.mramPtr - b.mramPtr;
    }

    friend bool operator< (MramPtr const& a, MramPtr const& b) { return a.mramPtr< b.mramPtr;  }
    friend bool operator> (MramPtr const& a, MramPtr const& b) { return b < a; }
    friend bool operator>=(MramPtr const& a, MramPtr const& b) { return !(a < b); }
    friend bool operator<=(MramPtr const& a, MramPtr const& b) { return !(b < a); }
};

////////////////////////////////////////////////////////////////////////////////
template<typename T>
class MRAMAllocator
{
    size_t offsetMRAM_ = 0;
    size_t offsetWRAM_ = 0;

public:
    using pointer       = MramPtr<T>;
    using const_pointer = MramPtr<T>;
    using void_pointer  = MramPtr<void>;
    using value_type    = T;

    pointer allocate(size_t n)
    {
        void* vpMRAM = (void*) (MRAM_buffer + offsetMRAM_);
        void* vpWRAM = (void*) (WRAM_buffer + offsetWRAM_);

        offsetMRAM_ += n*sizeof(T);
        offsetWRAM_ += 2*sizeof(T);

        printf ("[allocate] n: %ld  vpMRAM: 0x%lx   offset_: %d \n", uint64_t(n), uint64_t(vpMRAM), offsetMRAM_);

        return pointer ((T*)vpMRAM, (T*)vpWRAM, true);
    }

    void deallocate(pointer p, size_t n)
    {
        printf ("[deallocate] n: %ld\n", uint64_t(n));
    }

    bool operator==(const MRAMAllocator &) const { return true;  }
    bool operator!=(const MRAMAllocator &) const { return false; }
};

////////////////////////////////////////////////////////////////////////////////
struct banner
{
    static const char* line() { return "-------------------------------"; }
    const char* msg;
     banner (const char* m) : msg(m) {  printf ("%s BEFORE %-10s %s\n", line(), msg, line());  dumpWRAM<uint64_t>(); }
    ~banner ()                       {  printf ("%s AFTER  %-10s %s\n", line(), msg, line()); }
};

////////////////////////////////////////////////////////////////////////////////
void testvector (void)
{
#ifdef DPU
    if (me()==0)
#endif
    {
#if 1
        std::vector<uint64_t, MRAMAllocator<uint64_t>> v = {1,2,3,5,8};
#else
        std::vector<uint64_t> v = {1,2,3,5,8};
#endif
        {  banner b ("swap");
            auto& x = v[0];    // return *(this->_M_impl._M_start + __n);
        }
return;
        {  banner b ("reserve");    v.reserve(10);      }
        {  banner b ("push_back");  v.push_back (13);   }
        {  banner b ("loop");       for (auto x : v)  { printf ("-> %ld\n", uint64_t(x));   }  }
        {  banner b ("pop_back");   v.pop_back ();      }
        {  banner b ("pop_back");   v.pop_back ();      }
        {  banner b ("emplace_back");  v.emplace_back (21);   }
        {  banner b ("range loop");  for (auto x : v)  { printf ("-> %ld\n", uint64_t(x));   }  }
        {  banner b ("operator[]");  printf ("-> %ld\n", v[1]);   }
        {  banner b ("range loop");  for (auto x : v)  { printf ("-> %ld\n", uint64_t(x));   }  }
        {  banner b ("loop rev");    for (auto it=v.rbegin(); it!=v.rend(); ++it)  { printf ("-> %ld\n", uint64_t(*it));   }  }
        {  banner b ("loop");        for (auto it=v. begin(); it!=v. end(); ++it)  { printf ("-> %ld\n", uint64_t(*it));   }  }
        {  banner b ("erase");       v.erase (v.begin()+1, v.begin()+3); }
        {  banner b ("resize");     v.resize(10);  }
        {  banner b ("loop");       for (auto x : v)  { printf ("-> %ld\n", uint64_t(x));   }  }
        {  banner b ("resize");     v.resize(3);  }
        {  banner b ("loop");       for (auto x : v)  { printf ("-> %ld\n", uint64_t(x));   }  }
        {  banner b ("swap");       std::swap(v[0],v[1]);  }
        {  banner b ("loop");       for (auto x : v)  { printf ("-> %ld\n", uint64_t(x));   }  }
    }
}

////////////////////////////////////////////////////////////////////////////////
int main (void)
{
    testvector();
}
