////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/utils/metaprog.hpp>
#include <bpl/utils/tag.hpp>
#include <bpl/utils/splitter.hpp>
#include <bpl/arch/Arch.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

#ifndef DPU
    #define DEBUG_FMT "DPU=0 [%2d]  %s<%s>  sizeof(T)=%d\n"
#else
    #define DEBUG_FMT "DPU=1 [%2d]  %s<%s>  sizeof(T)=%d\n"
#endif

/** \brief Class for serialization.
 *
 * Implementation classes provide their result through an 'iterate' method that loops over
 * buffers (pointer+size). The 'to' method just glues all theses buffers in
 * a single one. Note however that it could be interesting to directly use 'iterate', for
 * instance in UPMEM context where the scatter/gather mechanism can take advantage of using
 * several buffers instead of using a single one (that possibly duplicates memory usage).
 *
 * NOTE: This serialization mini-framework doesn't aim at covering all cases; it should
 * just cope with simple types. It is also not possible to rely on an existing serialization
 * framework (boost, cereal, apalca, ...) because we need this framework to be used also on
 * PIM side (like UPMEM arch) where C++ is available but not the standard library.
 *
 * We mainly rely here on SFINAE + overloading (instead of template class specialization which is more verbose).
 *
 * \param[in] ARCH: architecture providing basic types (vector, array...)
 */

template<class ARCH, class BUFITER, int ROUNDUP>
struct Serialize
{
    // We retrieve type information from the provided architecture.
    USING(ARCH);

    using iter_t    = BUFITER;
    using buffer_t  = vector<char>;
    using pointer_t = typename BUFITER::pointer_t;
    using serial_size_t = uint64_t;

    ////////////////////////////////////////////////////////////////////////////////
    constexpr static auto round (int n)  {  return roundUp<ROUNDUP>(n); }

    // Returns a dummy buffer
    static auto getDummyBuffer()
    {
        static char dummy[8] = { 1,2,3,4,5,6,7,8 };
        return make_pair (dummy, sizeof(dummy));
    }

    ////////////////////////////////////////////////////////////////////////////////
    // SERIALIZABLE
    ////////////////////////////////////////////////////////////////////////////////
    template <typename T, typename FCT>
    requires (is_serializable_v<T>)
    static auto iterate (bool transient, int depth, const T& t, FCT fct, void* context=nullptr)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "is_serializable", (uint32_t) sizeof(T));

        // we forward the request to the implementation provided by the user
        serializable<T>::template iterate<ARCH,BUFITER,ROUNDUP> (transient, depth, t, fct, context);
    }

    template<typename T>
    requires (is_serializable_v<T>)
    static auto restore (iter_t& it, T& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  0, "restore", "is_serializable", (uint32_t) sizeof(T));

        // we forward the request to the implementation provided by the user
        serializable<T>::template restore<ARCH,BUFITER,ROUNDUP> (it, result);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // ARITHMETIC
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    requires (std::is_arithmetic_v<T>)
    static auto iterate (bool transient, int depth, const T& t, FCT fct, void* context=nullptr)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "is_arithmetic", (uint32_t) sizeof(T));
        fct (transient, depth, (void*)&t, sizeof(T), round(sizeof(T)));
    }

    template<typename T>
    requires (std::is_arithmetic_v<T>)
    static auto restore (iter_t& it, T& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  0, "restore", "is_arithmetic", (uint32_t) sizeof(T));

        result = it.template object<T> (round(sizeof(T)));

        it.advance (round(sizeof(T)) );

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // REF
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    static auto iterate (bool transient, int depth, std::reference_wrapper<T> t, FCT fct, void* context=nullptr)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "reference_wrapper", (uint32_t) sizeof(T));
        iterate (transient, depth, t.get(), fct, context);
    }

#if 0
    ////////////////////////////////////////////////////////////////////////////////
    // (std::is_trivially_copyable_v<T> and not (std::is_arithmetic_v<T> or is_serializable_v<T>))
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    requires (std::is_trivially_copyable_v<T> and not (std::is_arithmetic_v<T> or is_serializable_v<T>))
    static auto iterate (bool transient, int depth, const T& t, FCT fct, void* context=nullptr)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  depth, "iterate", "is_trivially_copyable and not is_arithmetic and not is_serializable_v", (uint32_t) sizeof(T));

        fct (transient, depth, (void*)&t, sizeof(T), round(sizeof(T)));
    }

    template<typename T>
    static auto restore (iter_t& it, T& result)
    requires (std::is_trivially_copyable_v<T> and not (std::is_arithmetic_v<T> or is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  0, "restore", "is_trivially_copyable and not is_arithmetic and not is_serializable_v",(uint32_t) sizeof(T));

        result = it.template object<T> (round(sizeof(T)));

        it.advance (round(sizeof(T)) );

        return true;
    }
#endif
    ////////////////////////////////////////////////////////////////////////////////
    // std::is_class_v<T> and not (std::is_trivially_copyable_v<T> or is_serializable_v<T>)
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    static auto iterate (bool transient, int depth, const T& t, FCT fct, void* context=nullptr)
    requires (std::is_class_v<T> and not (/*std::is_trivially_copyable_v<T> or*/ is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "not is_trivially_copyable and is_class", (uint32_t) sizeof(T));

        // We serialize each field of the incoming struct/class
        class_fields_iterate (t, [&] (auto&& field)
        {
            iterate (transient, depth, field, fct, context);
        });
    }

    template<typename T>
    static auto restore (iter_t& it, T& result)
    requires (std::is_class_v<T> and not (/*std::is_trivially_copyable_v<T> or*/ is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "not is_trivially_copyable and is_class", (uint32_t) sizeof(T));

        // NOTE: a preliminary version (1) created a tuple from the incoming object (a struct for instance),
        // (2) restored its content from the buffer and (3) converted the tuple back to the type of the incoming object.
        // => the main drawback was the necessary copy.
        // The possible choice is to restore directly each field of the struct, without using a tuple.
        // The consequence is that the code is not so pretty but we can avoid some unwanted copies then.
        class_fields_iterate (result, [&] (auto&& x)  { restore(it,x); });

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // ARRAY: not std::is_trivial_v<T>
    ////////////////////////////////////////////////////////////////////////////////

    // IMPORTANT... use size_t for the N template parameter (instead of int) otherwise
    // this template specialization won't be called and the default will be instead
    // (which leads to issue...)
    template<typename T, size_t N,typename FCT>
    static auto iterate (bool transient, int depth, const typename ARCH::template array<T,N>& t, FCT fct, void* context=nullptr)
    requires (not std::is_trivial_v<T>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  depth, "iterate", "array + not is_trivially_copyable", (uint32_t) N*sizeof(T));

        // We must iterate each item of the array.
        for (T&& x : t)  {  iterate (transient, depth, x,fct, context);  }
    }

    template<typename T, size_t N>
    static auto restore (iter_t& it, typename ARCH::template array<T,N>& result)
    requires (not std::is_trivial_v<T>)
    {
        uint32_t n = N*sizeof(T);

        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "array + not is_trivially_copyable", (uint32_t) n);

        it.memcpy (result, round (n) );

        // We initialize each item of the array.
        std::size_t idx=0;
        for (T& x : result)  {  it.initialize(x,idx++); }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // ARRAY: std::is_trivial_v<T>
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, size_t N,typename FCT>
    static auto iterate (bool transient, int depth, const typename ARCH::template array<T,N>& t, FCT fct, void* context=nullptr)
    requires (std::is_trivial_v<T>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "array + is_trivial_v<T>", (uint32_t) (N*sizeof(T)));

        uint32_t n = N*sizeof(T);
        fct (transient, depth, (void*)t.data(), n, round (n) );
    }

    template<typename T, size_t N>
    static auto restore (iter_t& it, typename ARCH::template array<T,N>& result)
    requires (std::is_trivial_v<T>)
    {
        uint32_t n = N*sizeof(T);

        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "array + is_trivial_v<T>", (uint32_t) n);

        it.memcpy (result.data(), round (n) );

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // PAIR
    ////////////////////////////////////////////////////////////////////////////////
    template<typename A, typename B, typename FCT>
    static auto iterate (bool transient, int depth, const pair<A,B>& t, FCT fct, void* context=nullptr)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "pair", (uint32_t) sizeof(pair<A,B>));

        iterate (true, depth+1, t.first, fct, context);
        iterate (true, depth+1, t.second,fct, context);
    }

    template<typename A, typename B>
    static auto restore (iter_t& it, pair<A,B>& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "pair", (uint32_t) sizeof(pair<A,B>));

        restore (it, result.first);
        restore (it, result.second);

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // TUPLE
    ////////////////////////////////////////////////////////////////////////////////
    template<typename ...ARGS, typename FCT>
    static void iterate (bool transient, int depth, const tuple<ARGS...>& t, FCT fct, void* context=nullptr)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "tuple", (uint32_t) sizeof(tuple<ARGS...>));
        for_each_in_tuple (t, [&] (auto&& x)
        {
            iterate (transient, depth+1, x,fct, context);
        });
    }

    template<typename ...ARGS>
    static auto restore (iter_t& it, tuple<ARGS...>& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "tuple", (uint32_t) sizeof(tuple<ARGS...>));

        for_each_in_tuple (result, [&it] (auto& x)
        {
            restore (it, x);
        });

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // STRING
    ////////////////////////////////////////////////////////////////////////////////
    template<typename FCT>
    static void iterate (bool transient, int depth, const string& t, FCT fct, void* context=nullptr)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "string", (uint32_t) size_t(0));

        serial_size_t n = t.size();

        iterate (true, depth, n,fct, context);

        if (n>0)  {  fct (transient, depth, (char*)t.data(), n, round(n)); }
    }

    static auto restore (iter_t& it, string& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "string", (uint32_t) size_t(0));

        // We retrieve the size.
        serial_size_t n = 0;
        restore (it, n);

        if (n>0)
        {
            // We setup the object.
            result.assign (it.get(),n);

            // We update the pointer.
            it.advance (n);
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // VECTOR: not std::is_trivially_copyable_v<T> and not is_serializable_v<vector<T>>
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    static auto iterate (bool transient, int depth, const vector<T>& t, FCT fct, void* context=nullptr)
    requires (not std::is_trivially_copyable_v<T> and not is_serializable_v<vector<T>>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "vector + not is_trivially_copyable", (uint32_t) sizeof(T));

        serial_size_t n = t.size();
        iterate (transient, depth, n,fct, context);

        for (auto&& x : t)  {  iterate (transient, depth+1, x,fct, context);  }
    }

    template<typename T>
    static auto restore (iter_t& it, vector<T>& result)
    requires (not std::is_trivially_copyable_v<T> and not is_serializable_v<vector<T>>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "vector + not is_trivially_copyable", (uint32_t) sizeof(T));

        // We get the vector size.
        serial_size_t n = 0;
        restore (it, n);

        // We resize the vector.
        result.resize (n);

        // We restore each item of the vector.
        for (size_t i=0; i<n; i++)  {  restore (it, result[i]);  }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // VECTOR: std::is_trivially_copyable_v<T>  and not is_serializable_v<vector<T>>
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    static auto iterate (bool transient, int depth, const vector<T>& t, FCT fct, void* context=nullptr)
    requires (std::is_trivially_copyable_v<T>  and not is_serializable_v<vector<T>>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "vector + is_trivially_copyable", (uint32_t) sizeof(T));

        serial_size_t n = t.size();
        iterate (true, depth, n,fct, context);

        if (n>0)
        {
            fct (transient, depth, (void*)t.data(), sizeof(T)*n, round(sizeof(T)*n));
        }
        else
        {
            // in case the vector is empty, we still provide some data in order not to have an nullptr.
            auto [dummy, N] = getDummyBuffer();
            fct (transient, depth, dummy, N, round(N));
        }

#ifdef WITH_SERIALIZATION_HASH_CHECK
        // we compute the sum of the hash for each item of the vector.
        uint64_t check = 0;
        for (auto const& x : t)  { check += get_hash(x); }
        iterate (true, depth, check, fct, context);
#endif
    }

    template<typename T>
    static auto restore (iter_t& it, vector<T>& result)
    requires (std::is_trivially_copyable_v<T>  and not is_serializable_v<vector<T>>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "vector + is_trivially_copyable", (uint32_t) sizeof(T));

        // We get the vector size.
        serial_size_t n = 0;
        restore (it, n);

        // We resize the vector.
        result.resize (n);

#if 0
        it.memcpy ((void*)result.data(), round(n*sizeof(T)));
        it.advance (round(n*sizeof(T)));
#else
        // We restore each item of the vector.
        for (size_t i=0; i<n; i++)  {  restore (it, result[i]);  }
#endif

#ifdef WITH_SERIALIZATION_HASH_CHECK
            uint64_t check1 = 0;
            restore (it, check1);
#endif

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // SPAN
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    static auto iterate (bool transient, int depth, const span<T>& t, FCT fct, void* context=nullptr)
    requires (not (std::is_trivially_copyable_v<T> or is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "span + not is_trivially_copyable", (uint32_t) sizeof(T));

        serial_size_t n = t.size();
        iterate (transient, depth, n,fct, context);

        [[maybe_unused]] uint64_t check = 0;

        if (n>0)
        {
            for (auto&& x : t)
            {
                iterate (transient, depth+1, x,fct, context);

#ifdef WITH_SERIALIZATION_HASH_CHECK
                // we compute the sum of the hash for each item of the vector.
                check += get_hash(x);
#endif
            }
        }
        else
        {
            // in case the vector is empty, we still provide some data in order not to have an nullptr.
            auto [dummy, N] = getDummyBuffer();
            fct (transient, depth, dummy, N, round(N));
        }

#ifdef WITH_SERIALIZATION_HASH_CHECK
        iterate (true, depth, check, fct, context);
#endif
    }

    template<typename T, typename FCT>
    static auto iterate (bool transient, int depth, const span<T>& t, FCT fct, void* context=nullptr)
    requires (std::is_trivially_copyable_v<T>  and not is_serializable_v<T>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "span + is_trivially_copyable", (uint32_t) sizeof(T));

        serial_size_t n = t.size();
        iterate (true, depth, n,fct);

        if (n>0)
        {
            fct (transient, depth, (void*)t.data(), sizeof(T)*t.size(), round(sizeof(T)*t.size()));
        }
        else
        {
            // in case the vector is empty, we still provide some data in order not to have an nullptr.
            auto [dummy, N] = getDummyBuffer();
            fct (transient, depth, dummy, N, round(N));
        }

#ifdef WITH_SERIALIZATION_HASH_CHECK
        // we compute the sum of the hash for each item of the vector.
        uint64_t check = 0;
        for (auto const& x : t)  { check += get_hash(x); }
        iterate (true, depth, check, fct, context);
#endif
    }

    template<typename T>
    static auto restore (iter_t& it, span<T>& result)
    requires (not (std::is_trivially_copyable_v<T> or is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "span + not is_trivially_copyable", (uint32_t) sizeof(T));

        // We get the vector size.
        serial_size_t n = 0;
        restore (it, n);
        DEBUG_SERIALIZATION ("RESTORE(span + not is_trivially_copyable + not is_serializable_v):  size: %d\n", n);

        T item;
        // We restore each item of the vector.
        for (size_t i=0; i<n; i++)  {  restore (it, item);  }

//        // We resize the vector.
//        result.resize (n);
//
//        // We restore each item of the vector.
//        for (size_t i=0; i<n; i++)  {  restore (it, result[i]);  }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // End user API
    ////////////////////////////////////////////////////////////////////////////////

    template<typename T, typename CALLBACK>
    static auto size (const T& x, CALLBACK callback)
    {
        DEBUG_SERIALIZATION ("[size] ++++++++++++++++++\n");

        size_t idx = 0;
        pair<size_t,size_t> sizes {0,0};  // transient,permanent
        iterate (false, 0, x, [&] (bool transient, int depth, void* ptr, size_t size, size_t roundedSize)
        {
            callback (transient, size, roundedSize);
            sizes.first  +=     transient ?  roundedSize : 0 ;
            sizes.second += not transient ?  roundedSize : 0 ;
            idx++;
        });
        DEBUG_SERIALIZATION ("[size] ------------------  %d,%d \n", uint32_t(sizes.first),uint32_t(sizes.first)  );
        return sizes;
    }

    template<typename T>
    static auto size (const T& x)
    {
        return size (x, [] (bool transient, size_t size, size_t roundedSize) {} );
    }

    template<typename T>
    static buffer_t to (const T& x)
    {
        // First iteration: get the total size.
        // Note: we also could use only one iteration and resize the vector size at each iteration.
        // -> however, computing first the full vector size allows to have only one vector memory allocation.
        auto totalSize = size (x);

        DEBUG_SERIALIZATION ("[to  ] ++++++++++++++++++\n");

        // Second iteration: fill the buffer
        buffer_t buffer (totalSize.first + totalSize.second);
        auto loop = buffer.data();
        iterate (false, 0, x, [&] (bool transient, int depth, void* ptr, size_t size, size_t roundedSize)
        {
            DEBUG_SERIALIZATION ("[to  ]  ------------------------------------------> dist: %ld\n", loop - buffer.data());
            // We can do a memcpy here since the data pointed by 'ptr' is supposed
            // to be available at this time in memory. We are also sure about the size
            // available in 'buffer' since its size has been previously computed.
            memcpy (loop, (char*)ptr, size);
            loop += roundedSize;
        });

        return buffer;
    }

    template<typename INFO, typename TRANSFO, typename CALLBACK, typename ...ARGS>
    static size_t tuple_to_buffer (const tuple<ARGS...>& x, buffer_t& buffer, INFO info, TRANSFO transfo, CALLBACK cbk)
    {
        // The serialized buffer is built by serializing each item of the provided tuple.
        // A previous version computed first the size of the buffer in order to allocate
        // the correct size. Now, we prefer to build the buffer dynamically and update the
        // buffer size when needed. Actually, we provided a initial 'defaultSize' size that
        // could be enough to cope with several small sizeof for the tuple items.
        [[maybe_unused]] size_t defaultSize = 256;

        size_t idxArg          = 0;
        size_t offsetLocal     = 0;
        size_t offsetGlobal    = 0;
        size_t offsetTransient = 0;

        // We need to compute the buffer size first since we will provide addresses to the 'cbk' and not offsets
        // from a base address (that could change over time if the buffer needs to be resized).
        // The buffer size should include only the size of transient objects. For instance, arguments that are vectors
        // are supposed to exist during the whole serialization process, so their data don't need to be copied in the
        // buffer. On the other hand, the size of a vector requires the allocation of a temporary object that holds the
        // vector size; in such a case, the object is transient and needs to be copied in the buffer.

        size_t transientBufferSize = 0;

        size_t iarg=0;
        for_each_in_tuple (x, [&] (auto&& itemTuple)
        {
            // see https://stackoverflow.com/questions/46114214/lambda-implicit-capture-fails-with-variable-declared-from-structured-binding
            size_t level, isSplit, nbUnits;
            std::tie(level, isSplit, nbUnits) = info (iarg, std::forward<decltype(itemTuple)>(itemTuple));

            // We get the size of the current argument.
            size (itemTuple, [&] (bool transient, size_t size, size_t roundedSize)
            {
                if (transient) {  transientBufferSize += roundedSize * nbUnits;  }
            });

            iarg++;
        });

        // We reserve only for transient data
        buffer.resize (transientBufferSize);

        // We need to know the number of parts iterated for each argument.
        // These numbers will be known for sure after the parsing of the first DPU and then can
        // be reused for other arguments as an offet.
        size_t nbPartsPerArgOffset[32];  for (auto& x : nbPartsPerArgOffset)  { x=0; }

        for_each_in_tuple (x, [&] (auto&& itemTuple)
        {
            DEBUG_SERIALIZATION ("[tuple_to_buffer  ]  ------------------------------------------> idxArg:%ld  local: %ld  global: %ld\n", idxArg, offsetLocal, offsetGlobal);

            // For each item of the tuple, we apply a transformation.
            // Such a transformation may for instance split the item in several parts (one per DPU for instance).
            size_t idxDPU = 0;

            auto fct = [&] (bool status, auto&& item)
            {
                DEBUG_SERIALIZATION ("[tuple_to_buffer  ]      status: %d\n", status);

                size_t idxPart = 0;  // for instance, a vector is serialized in two parts.

                // We serialize the item only if the status allows to do so.
                if (status)
                {
                    offsetLocal = 0;

                    // lifecycle object :  true-> transient   false->permanent
                    iterate (false, 0, item,  [&] (bool isTransient, int depth, void* ptr, size_t size, size_t roundedSize)
                    {
                        // Here, we are in the following loop:
                        //    foreach idxArg
                        //       foreach idxDPU
                        //          foreach idxPart
                        //
                        // So we have to compute the position of the current block (in the scatter/gather DPU point of view)
                        // for the current DPU. Example:
                        //
                        //    DPU0:  ARG0            ARG1       ARG2
                        //           PART0 PART1     PART0      PART0 PART1
                        //    DPU1:  ARG0            ARG1       ARG2
                        //           PART0 PART1     PART0      PART0 PART1

                        size_t idxBlock = nbPartsPerArgOffset[idxArg] + idxPart;

                        uint8_t* actualPtr = (uint8_t*) ptr;

                        if (isTransient)
                        {
                            // We may have to resize the buffer if needed. Note that we add a few extra 'defaultSize' bytes

                            // We can do a memcpy here since the data pointed by 'ptr' is supposed
                            // to be available at this time in memory. We are also sure about the size
                            // available in 'buffer' since its size has been previously updated if needed.
                            memcpy (buffer.data() + offsetTransient, (char*)ptr, size);

                            actualPtr = (uint8_t*) buffer.data() + offsetTransient;

                            offsetTransient += roundedSize;
                        }

                        VERBOSE_SERIALIZATION ("[tuple_to_buffer]  transient: %d sts: %d  idxDPU: %3ld  idxArg: %3ld  idxPart: %2ld  idxBlock: %3ld  nbPtsOff: %ld  offsets: [%10ld,%7ld,%7ld]  #buf: %10d  size: %8d  ptr: [%p,%p] \n",
                            isTransient, status, idxDPU, idxArg, idxPart, idxBlock, nbPartsPerArgOffset[idxArg],
                            offsetGlobal, offsetLocal,offsetTransient,
                            (uint32_t)buffer.size(),
                            (uint32_t)size, ptr, actualPtr
                        );

                        // We call the callback in order to provide some information during the serialization process.
                        // NOTE: we could have used a std::vector in order to gather this information and then provide it
                        // to the caller. However, the serialize capacity we implement here can be used either by the multicore
                        // arch and the UPMEM arch, so we prefer here make no use of explicit std class but nevertheless provide
                        // some information through a callback.

                        // We invoke the callback for the current part.
                        cbk (idxDPU, idxArg, idxBlock, actualPtr, roundedSize);

                        offsetLocal  += roundedSize;
                        offsetGlobal += roundedSize;

                        // We increase the parts number for the current argument to be serialized.
                        idxPart++;
                    });
                }

                else  // not status
                {
                    //printf ("==> iterate  sts: %d  idxDPU: %3ld  idxArg: %3ld  \n", status, idxDPU, idxArg);

                    // We have to reuse the same scheme as the previous DPU.
                    // By convention, we set the pointer to null in order to inform the caller

                    for (size_t idxBlock = nbPartsPerArgOffset[idxArg]; idxBlock<nbPartsPerArgOffset[idxArg+1]; idxBlock++)
                    {
                        cbk (idxDPU, idxArg, idxBlock, nullptr, 0);
                    }
                }

                // We remember the number of iterated parts for the current argument.
                // We can do that on the first DPU for instance.
                if (idxDPU==0)
                {
                    // We need to compute the sum of the previous values.
                    nbPartsPerArgOffset [idxArg+1] = idxPart + nbPartsPerArgOffset [idxArg];
                }

                idxDPU++;
            };

            // We get some information about the current item of the tuple.
            // In particular, we are interested to know whether it has to be split or not.
            auto [level, isSplit, nbUnits] = info (idxArg, std::forward<decltype(itemTuple)>(itemTuple));

            DEBUG_SERIALIZATION ("[tuple_to_buffer  ]  level: %d   isSplit: %d  nbUnits: %ld \n", level, isSplit, nbUnits);

            // According to the split status, we don't have the same action.
            if (not isSplit)
            {
                // No split -> we can use the same item.
                // For instance with Upmem arch, it means that the same information will be broadcasted to all DPUs.
                for (size_t i=0; i<nbUnits; i++)  {  fct (i==0, std::forward<decltype(itemTuple)>(itemTuple));  }
            }
            else
            {
                // We need a static check here -> avoid to call this code for non 'splitable' types (otherwise compil error).
                // This is actually not very satisfying and should be improved.
                if constexpr(is_splitable_v<decltype(itemTuple)>)
                {
                    bool status = true;

                    // Split -> several items will be created (one per split part)
                    for (auto&& itemRef : transfo(idxArg, std::forward<decltype(itemTuple)>(itemTuple)))
                    {
                        fct (status, std::forward<decltype(itemRef)>(itemRef));
                    }
                }
            }

            idxArg++;
        });

        size_t idx = 0;
        for (__attribute__((unused)) unsigned char c : buffer)
        {
            if (idx % 32 == 0)  { VERBOSE_SERIALIZATION ("\nhost2dpu 0x%04lx> ", idx); }
            VERBOSE_SERIALIZATION ("%4d ", c);
            if (idx % 8 == 7) {  VERBOSE_SERIALIZATION ("|"); }
            idx++;
        }
        VERBOSE_SERIALIZATION ("\n");

        return offsetGlobal;
    }


    template<typename T>
    static T from (const buffer_t& buf)
    {
        iter_t it { (pointer_t)buf.data() };
        T result;
        restore (it, result);
        return result;
    }

    template<typename T>
    static T from (const pointer_t& buf)
    {
        iter_t it {buf};
        T result;
        restore (it, result);
        return result;
    }

    template<typename T>
    static void from (const pointer_t& buf, T& result)
    {
        iter_t it {buf};
        restore (it, result);
    }

    template<typename T>
    static T identity (const T& x)
    {
        return from<T> (to<T> (x));
    }
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#include <bpl/utils/splitter.hpp>

// We need to make SplitProxy a serializable -> just forward to the inner attribute '_t'
template<typename L, bpl::SplitKind K, typename TT>
struct bpl::serializable<details::SplitProxy<L,K,TT>>
{
    // we tell that our structure can be serialized
    static constexpr int value = true;

    template<class ARCH, class BUFITER, int ROUNDUP, typename T, typename FCT>
    static auto iterate (bool transient, int depth, const T& t, FCT fct, void* context=nullptr)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (transient, depth+1, t._t, fct, context);
    }

    template<class ARCH, class BUFITER, int ROUNDUP, typename T>
    static auto restore (BUFITER& it, T& result)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, result._t);
    }
};

////////////////////////////////////////////////////////////////////////////////
#include <bpl/utils/Range.hpp>

template<>
struct bpl::serializable<bpl::Range>
{
    // we tell that our structure can be serialized
    static constexpr int value = true;

    template<class ARCH, class BUFITER, int ROUNDUP, typename T, typename FCT>
    static auto iterate (bool transient, int depth, const T& t, FCT fct, void* context=nullptr)
    {
        //printf ("Range::iterate:  %ld %ld \n", *t.begin(), *t.end  ());
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (true, depth+1, *t.begin(), fct, context);
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (true, depth+1, *t.end  (), fct, context);
    }

    template<class ARCH, class BUFITER, int ROUNDUP, typename T>
    static auto restore (BUFITER& it, T& result)
    {
        Range::type a,b;
        Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, a);
        Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, b);
        result = Range (a,b);
        //printf ("Range::restore:  %ld %ld \n", a, b);
    }
};


template<>
struct bpl::serializable<bpl::RakeRange>
{
    // we tell that our structure can be serialized
    static constexpr int value = true;

    template<class ARCH, class BUFITER, int ROUNDUP, typename T, typename FCT>
    static auto iterate (bool transient, int depth, const T& t, FCT fct, void* context=nullptr)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (transient, depth+1, *t.begin(), fct, context);
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (transient, depth+1, *t.end  (), fct, context);
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (transient, depth+1,  t.begin().modulo(), fct, context);
        //printf ("RakeRange::iterate:  %ld %ld %ld\n", *t.begin(), *t.end  (),  t.begin().modulo());

    }

    template<class ARCH, class BUFITER, int ROUNDUP, typename T>
    static auto restore (BUFITER& it, T& result)
    {
        RakeRange::type a,b,c;
        Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, a);
        Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, b);
        Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, c);
        //printf ("RakeRange::restore:  %ld %ld %ld\n", a, b, c);
        result = RakeRange (a,b,c);
    }
};

////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct bpl::serializable<bpl::once<T>> : std::true_type
{
    template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE, typename FCT>
    static auto iterate (bool transient, int depth, const TYPE& t, FCT fct, void* context=nullptr)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (transient, depth+1, *t, fct, context);
    }

    template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE>
    static auto restore (BUFITER& it, TYPE& result)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, *result);
    }
};
