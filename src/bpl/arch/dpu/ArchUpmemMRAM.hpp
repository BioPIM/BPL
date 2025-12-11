////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

////////////////////////////////////////////////////////////////////////////////
#ifdef DPU  // THIS PART SHOULD BE COMPILED ONLY FOR DPU
////////////////////////////////////////////////////////////////////////////////

#pragma once

extern "C"
{
    #include <mram.h>
    #include <mram_unaligned.h>
    #include <mutex.h>

    void* mram_read_unaligned  (const __mram_ptr void *from, void *buffer, unsigned int nb_of_bytes);
    void  mram_write_unaligned (const void *from, __mram_ptr void *dest, unsigned nb_of_bytes);
}

extern const mutex_id_t  __MRAM_Allocator_mutex__;

////////////////////////////////////////////////////////////////////////////////
template<typename T>  struct MRAMWrite{};
template<typename T>  struct MRAMRead {};

template<> struct MRAMWrite<uint32_t>
{ auto operator() (void* dest, void* data, size_t sizeInBytes)  { mram_write_unaligned (data, (__mram_ptr void*)dest, sizeInBytes);  }};

template<> struct MRAMRead<uint32_t>
{ auto operator() (void* from, void* data, size_t sizeInBytes)  { mram_read_unaligned ((__mram_ptr void*)from, data, sizeInBytes);  }};

template<> struct MRAMWrite<uint64_t>
{ auto operator() (void* dest, void* data, size_t sizeInBytes)  { mram_write (data, (__mram_ptr void*)dest, sizeInBytes);  }};

template<> struct MRAMRead<uint64_t>
{ auto operator() (void* from, void* data, size_t sizeInBytes)  { mram_read ((__mram_ptr void*)from, data, sizeInBytes);  }};

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

struct MRAM
{
    // using address_t = uint64_t;
    using address_t    = uint32_t;
    
    using rw_address_t = uint64_t;
    // using rw_address_t = uint32_t;

    template<bool LOCK>
    class Allocator
    {
    private:

        using pointer_t = __mram_ptr void*;

    public:

        MRAMRead <rw_address_t> reader_;
        MRAMWrite<rw_address_t> writer_;

        void init (address_t initAdd)
        {
            if (LOCK)  {   mutex_lock (__MRAM_Allocator_mutex__);  }
            init_ = start_ = data_ = (pointer_t) initAdd;
            if (LOCK)  {   mutex_unlock (__MRAM_Allocator_mutex__); }
        }

        void reset()
        {
            if (LOCK)  {   mutex_lock (__MRAM_Allocator_mutex__);  }

            data_ = start_;

            if (LOCK)  {   mutex_unlock (__MRAM_Allocator_mutex__); }
        }

        address_t get (size_t sizeInBytes)
        {
            if (LOCK)  {   mutex_lock (__MRAM_Allocator_mutex__);  }

            pointer_t result = data_;

            data_ = (pointer_t)  ((address_t)(uint8_t*)data_ + sizeInBytes);

            nbCallsGet += 1;

            if (LOCK)  {   mutex_unlock (__MRAM_Allocator_mutex__);  }

            return address_t (result);
        }

        /** */
        address_t writeAt (void* dest, void* data, size_t sizeInBytes)
        {
            // We might have to fallback on unaligned version
            if (sizeInBytes%8==0) {  writer_ (dest, data, sizeInBytes);  }
            else { mram_write_unaligned (data, (__mram_ptr void*)dest, sizeInBytes);  }

            return (address_t) dest;
        }

        auto writeAtomic (address_t tgt, address_t const& src)
        {
            mram_write_int_atomic ((uint32_t)tgt, (uint32_t)src);
        }

        /** */
        address_t write (void* data, size_t sizeInBytes)
        {
            address_t result = get (sizeInBytes);

            writer_ ((void*)result, data, sizeInBytes);
            return result;
        }

        /** */
        void read (void* from, void* data, size_t sizeInBytes)
        {
if (LOCK)  {   mutex_lock (__MRAM_Allocator_mutex__);  }

            // Warning !!! not protected via mutex (for perf concern), so will provide approximation only.
            nbCallsRead += 1;
if (LOCK)  {   mutex_unlock (__MRAM_Allocator_mutex__);  }

            reader_ (from, data, sizeInBytes);
        }

        /** */
        template<typename T>
        address_t write (const T& data)
        {
            return write ((void*)&data, sizeof(data));
        }

        template<typename T>
        address_t writeAt (void* dest, const T& data)
        {
            return writeAt (dest, (void*)&data, sizeof(data));
        }

        /** */
        template<typename T>
        void read (void* from, const T& data)
        {
            return read (from, (void*)&data, sizeof(data));
        }

        uint32_t used() const {  return (uint8_t*)data_ - (uint8_t*)start_; }

        uint32_t pos() const { return (uint32_t) (uint8_t*)data_; }

        uint32_t start() const { return (uint32_t) (uint8_t*)start_; }

        pointer_t init_  = 0;
        pointer_t start_ = 0;
        pointer_t data_  = 0;

        size_t nbCallsGet  = 0;
        size_t nbCallsRead = 0;
    };
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////

extern bpl::MRAM::Allocator<true>  __MRAM_Allocator_lock__;
extern bpl::MRAM::Allocator<false> __MRAM_Allocator_nolock__;

////////////////////////////////////////////////////////////////////////////////
#endif  // #ifdef DPU
////////////////////////////////////////////////////////////////////////////////
