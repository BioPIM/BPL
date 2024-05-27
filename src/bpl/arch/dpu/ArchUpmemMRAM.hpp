////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

////////////////////////////////////////////////////////////////////////////////
#ifdef DPU  // THIS PART SHOULD BE COMPILED ONLY FOR DPU
////////////////////////////////////////////////////////////////////////////////

#ifndef __BPL_ARCH_MRAM__
#define __BPL_ARCH_MRAM__
////////////////////////////////////////////////////////////////////////////////

extern "C"
{
    #include <mram.h>
    #include <mutex.h>
}

extern const mutex_id_t  __MRAM_Allocator_mutex__;

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace arch {
////////////////////////////////////////////////////////////////////////////////

struct MRAM
{
    using address_t = uint64_t;

    template<bool LOCK>
    class Allocator
    {
    private:

        using pointer_t = __mram_ptr void*;

    public:

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

            data_ = (pointer_t) ((uint8_t*)data_ + sizeInBytes);

            nbCallsGet += 1;

            if (LOCK)  {   mutex_unlock (__MRAM_Allocator_mutex__);  }

            return address_t (result);
        }

        /** */
        address_t writeAt (address_t dest, void* data, size_t sizeInBytes)
        {
            mram_write (data, (pointer_t)dest, sizeInBytes);
            return dest;
        }

        /** */
        address_t write (void* data, size_t sizeInBytes)
        {
            address_t result = get (sizeInBytes);

            mram_write (data, (pointer_t)result, sizeInBytes);
            return result;
        }

        /** */
        void read (address_t from, void* data, size_t sizeInBytes)
        {
if (LOCK)  {   mutex_lock (__MRAM_Allocator_mutex__);  }

            // Warning !!! not protected via mutex (for perf concern), so will provide approximation only.
            nbCallsRead += 1;
if (LOCK)  {   mutex_unlock (__MRAM_Allocator_mutex__);  }

            mram_read ((pointer_t)from, data, sizeInBytes);

        }

        /** */
        template<typename T>
        address_t write (const T& data)
        {
            return write ((void*)&data, sizeof(data));
        }

        template<typename T>
        address_t writeAt (address_t dest, const T& data)
        {
            return writeAt (dest, (void*)&data, sizeof(data));
        }

        /** */
        template<typename T>
        void read (address_t from, const T& data)
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
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

extern bpl::arch::MRAM::Allocator<true>  __MRAM_Allocator_lock__;
extern bpl::arch::MRAM::Allocator<false> __MRAM_Allocator_nolock__;

////////////////////////////////////////////////////////////////////////////////
#endif // __BPL_ARCH_MRAM__

////////////////////////////////////////////////////////////////////////////////
#endif  // #ifdef DPU
////////////////////////////////////////////////////////////////////////////////
