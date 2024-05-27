////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_UTILS_SERIALIZATION_HPP__
#define __BPL_UTILS_SERIALIZATION_HPP__

#include <bpl/utils/metaprog.hpp>
#include <bpl/utils/tag.hpp>
#include <bpl/arch/Arch.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

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

    ////////////////////////////////////////////////////////////////////////////////
    static auto round (int n)  {  return roundUp<ROUNDUP>(n); }

    ////////////////////////////////////////////////////////////////////////////////
    // SERIALIZABLE
    ////////////////////////////////////////////////////////////////////////////////
    template <typename T, typename FCT>
    requires (is_serializable_v<T>)
    static auto iterate (int depth, const T& t, FCT fct)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "is_serializable", (uint32_t) sizeof(T));

        // we forward the request to the implementation provided by the user
        serializable<T>::template iterate<ARCH,BUFITER,ROUNDUP> (depth, t, fct);
    }

    template<typename T>
    requires (is_serializable_v<T>)
    static auto restore (iter_t& it, T& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  0, "restore", "is_serializable", (uint32_t) sizeof(T));

        // we forward the request to the implementation provided by the user
        serializable<T>::template restore<ARCH,BUFITER,ROUNDUP> (it, result);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // ARITHMETIC
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    requires (std::is_arithmetic_v<T>)
    static auto iterate (int depth, const T& t, FCT fct)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "is_arithmetic", (uint32_t) sizeof(T));
        fct (depth, (void*)&t, sizeof(T), round(sizeof(T)));
    }

    template<typename T>
    requires (std::is_arithmetic_v<T>)
    static auto restore (iter_t& it, T& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  0, "restore", "is_arithmetic", (uint32_t) sizeof(T));

        result = it.template object<T> (round(sizeof(T)));

        it.advance (round(sizeof(T)) );
    }

    ////////////////////////////////////////////////////////////////////////////////
    // (std::is_trivially_copyable_v<T> and not (std::is_arithmetic_v<T> or is_serializable_v<T>))
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    requires (std::is_trivially_copyable_v<T> and not (std::is_arithmetic_v<T> or is_serializable_v<T>))
    static auto iterate (int depth, const T& t, FCT fct)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  depth, "iterate", "is_trivially_copyable and not is_arithmetic and not is_serializable_v", (uint32_t) sizeof(T));

        fct (depth, (void*)&t, sizeof(T), round(sizeof(T)));
    }

    template<typename T>
    static auto restore (iter_t& it, T& result)
    requires (std::is_trivially_copyable_v<T> and not (std::is_arithmetic_v<T> or is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  0, "restore", "is_trivially_copyable and not is_arithmetic and not is_serializable_v",(uint32_t) sizeof(T));

        result = it.template object<T> (round(sizeof(T)));

        it.advance (round(sizeof(T)) );
    }

    ////////////////////////////////////////////////////////////////////////////////
    // std::is_class_v<T> and not (std::is_trivially_copyable_v<T> or is_serializable_v<T>)
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    static auto iterate (int depth, const T& t, FCT fct)
    requires (std::is_class_v<T> and not (std::is_trivially_copyable_v<T> or is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "not is_trivially_copyable and is_class", (uint32_t) sizeof(T));

        // We convert the incoming class into a tuple.
        iterate (depth, to_tuple(t), fct);
    }

    template<typename T>
    static auto restore (iter_t& it, T& result)
    requires (std::is_class_v<T> and not (std::is_trivially_copyable_v<T> or is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "not is_trivially_copyable and is_class", (uint32_t) sizeof(T));

        // NOTE: a preliminary version (1) created a tuple from the incoming object (a struct for instance),
        // (2) restored its content from the buffer and (3) converted the tuple back to the type of the incoming object.
        // => the main drawback was the necessary copy.
        // The possible choice is to restore directly each field of the struct, without using a tuple.
        // The consequence is that the code is not so pretty but we can avoid some unwanted copies then.
        using type = std::decay_t<T>;

        if constexpr(is_braces_constructible<type, any_type, any_type, any_type, any_type, any_type>{})
        {
          auto& [p1, p2, p3, p4, p5] = result;
          restore (it, p1);  restore (it, p2); restore (it, p3); restore (it, p4); restore (it, p5);
        }
        if constexpr(is_braces_constructible<type, any_type, any_type, any_type, any_type>{})
        {
          auto& [p1, p2, p3, p4] = result;
          restore (it, p1);  restore (it, p2); restore (it, p3); restore (it, p4);
        }
        else if constexpr(is_braces_constructible<type, any_type, any_type, any_type>{})
        {
          auto& [p1, p2, p3] = result;
          restore (it, p1);  restore (it, p2); restore (it, p3);
        }
        else if constexpr(is_braces_constructible<type, any_type, any_type>{})
        {
          auto& [p1, p2] = result;
          restore (it, p1);  restore (it, p2);

        }
        else if constexpr(is_braces_constructible<type, any_type>{})
        {
          auto& [p1] = result;  restore (it, p1);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // ARRAY: not std::is_trivial_v<T>
    ////////////////////////////////////////////////////////////////////////////////

    // IMPORTANT... use size_t for the N template parameter (instead of int) otherwise
    // this template specialization won't be called and the default will be instead
    // (which leads to issue...)
    template<typename T, size_t N,typename FCT>
    static auto iterate (int depth, const typename ARCH::template array<T,N>& t, FCT fct)
    requires (not std::is_trivial_v<T>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT,  depth, "iterate", "array + not is_trivially_copyable", (uint32_t) N*sizeof(T));

        // We must iterate each item of the array.
        for (T&& x : t)  {  iterate (depth, x,fct);  }
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
    }

    ////////////////////////////////////////////////////////////////////////////////
    // ARRAY: std::is_trivial_v<T>
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, size_t N,typename FCT>
    static auto iterate (int depth, const typename ARCH::template array<T,N>& t, FCT fct)
    requires (std::is_trivial_v<T>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "array + not is_serializable_v<T>", (uint32_t) (N*sizeof(T)));

        uint32_t n = N*sizeof(T);
        fct (depth, (void*)t.data(), n, round (n) );
    }

    template<typename T, size_t N>
    static auto restore (iter_t& it, typename ARCH::template array<T,N>& result)
    requires (std::is_trivial_v<T>)
    {
        uint32_t n = N*sizeof(T);

        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "array + not is_serializable_v<T>", (uint32_t) n);

        it.memcpy (result.data(), round (n) );
    }

    ////////////////////////////////////////////////////////////////////////////////
    // PAIR
    ////////////////////////////////////////////////////////////////////////////////
    template<typename A, typename B, typename FCT>
    static auto iterate (int depth, const pair<A,B>& t, FCT fct)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "pair", (uint32_t) sizeof(pair<A,B>));

        iterate (depth+1, t.first, fct);
        iterate (depth+1, t.second,fct);
    }

    template<typename A, typename B>
    static auto restore (iter_t& it, pair<A,B>& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "pair", (uint32_t) sizeof(pair<A,B>));

        restore (it, result.first);
        restore (it, result.second);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // TUPLE
    ////////////////////////////////////////////////////////////////////////////////
    template<typename ...ARGS, typename FCT>
    static void iterate (int depth, const tuple<ARGS...>& t, FCT fct)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "tuple", (uint32_t) sizeof(tuple<ARGS...>));
        for_each_in_tuple (t, [fct,depth] (auto&& x)
        {
            iterate (depth+1, x,fct);
        });
    }

    template<typename ...ARGS>
    static void restore (iter_t& it, tuple<ARGS...>& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "tuple", (uint32_t) sizeof(tuple<ARGS...>));

        for_each_in_tuple (result, [&it] (auto& x)
        {
            restore (it, x);
        });
    }

    ////////////////////////////////////////////////////////////////////////////////
    // STRING
    ////////////////////////////////////////////////////////////////////////////////
    template<typename FCT>
    static void iterate (int depth, const string& t, FCT fct)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "string", (uint32_t) size_t(0));

        typename string::size_type n = t.size();
        iterate (depth, n,fct);
        fct (depth, (char*)t.data(), n, round(n));
    }

    static void restore (iter_t& it, string& result)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "string", (uint32_t) size_t(0));

        // We retrieve the size.
        size_t n = 0;
        restore (it, n);

        // We setup the object.
        result.assign (it.get(),n);

        // We update the pointer.
        it.advance (n);
    }

    ////////////////////////////////////////////////////////////////////////////////
    // VECTOR: not std::is_trivially_copyable_v<T> and not is_serializable_v<vector<T>>
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    static auto iterate (int depth, const vector<T>& t, FCT fct)
    requires (not std::is_trivially_copyable_v<T> and not is_serializable_v<vector<T>>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "vector + not is_trivially_copyable", (uint32_t) sizeof(T));

        typename vector<T>::size_type n = t.size();
        iterate (depth, n,fct);

        for (auto&& x : t)  {  iterate (depth+1, x,fct);  }
    }

    template<typename T>
    static auto restore (iter_t& it, vector<T>& result)
    requires (not std::is_trivially_copyable_v<T> and not is_serializable_v<vector<T>>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "vector + not is_trivially_copyable", (uint32_t) sizeof(T));

        // We get the vector size.
        size_t n = 0;
        restore (it, n);

        // We resize the vector.
        result.resize (n);

        // We restore each item of the vector.
        for (size_t i=0; i<n; i++)  {  restore (it, result[i]);  }
    }

    ////////////////////////////////////////////////////////////////////////////////
    // VECTOR: std::is_trivially_copyable_v<T>  and not is_serializable_v<vector<T>>
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    static auto iterate (int depth, const vector<T>& t, FCT fct)
    requires (std::is_trivially_copyable_v<T>  and not is_serializable_v<vector<T>>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "vector + is_trivially_copyable", (uint32_t) sizeof(T));

        typename vector<T>::size_type n = t.size();
        iterate (depth, n,fct);

        fct (depth, (void*)t.data(), sizeof(T)*t.size(), round(sizeof(T)*t.size()));
    }

    template<typename T>
    static auto restore (iter_t& it, vector<T>& result)
    requires (std::is_trivially_copyable_v<T>  and not is_serializable_v<vector<T>>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "vector + is_trivially_copyable", (uint32_t) sizeof(T));

        // We get the vector size.
        size_t n = 0;
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
    }

    ////////////////////////////////////////////////////////////////////////////////
    // SPAN
    ////////////////////////////////////////////////////////////////////////////////
    template<typename T, typename FCT>
    static auto iterate (int depth, const span<T>& t, FCT fct)
    requires (not (std::is_trivially_copyable_v<T> or is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "span + not is_trivially_copyable", (uint32_t) sizeof(T));

        DEBUG_SERIALIZATION ("ITERATE(span + not is_trivially_copyable):  size: %d\n", t.size());

        typename vector<T>::size_type n = t.size();
        iterate (depth, n,fct);

        for (auto&& x : t)  {  iterate (depth+1, x,fct);  }
    }

    template<typename T, typename FCT>
    static auto iterate (int depth, const span<T>& t, FCT fct)
    requires (std::is_trivially_copyable_v<T>  and not is_serializable_v<T>)
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, depth, "iterate", "span + is_trivially_copyable", (uint32_t) sizeof(T));
        DEBUG_SERIALIZATION ("ITERATE(span + is_trivially_copyable):  size: %d\n", t.size());
    }

    template<typename T>
    static auto restore (iter_t& it, span<T>& result)
    requires (not (std::is_trivially_copyable_v<T> or is_serializable_v<T>))
    {
        DEBUG_SERIALIZATION (DEBUG_FMT, 0, "restore", "span + not is_trivially_copyable", (uint32_t) sizeof(T));

        // We get the vector size.
        size_t n = 0;
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
    }

    ////////////////////////////////////////////////////////////////////////////////
    // End user API
    ////////////////////////////////////////////////////////////////////////////////

    template<typename T, typename CALLBACK>
    static size_t size (const T& x, CALLBACK callback)
    {
        DEBUG_SERIALIZATION ("[size] ++++++++++++++++++\n");

        size_t idx = 0;
        size_t totalSize = 0;
        iterate (0, x, [&] (int depth, void* ptr, size_t size, size_t roundedSize)
        {
            callback (depth, idx, totalSize);
            totalSize += roundedSize;
            idx++;
        });
        DEBUG_SERIALIZATION ("[size] ------------------  %d \n", uint32_t(totalSize) );
        return totalSize;
    }

    template<typename T>
    static size_t size (const T& x)
    {
        return size (x, [] (int depth, size_t idx, size_t offset) {} );
    }

    template<typename T>
    static buffer_t to (const T& x)
    {
        // First iteration: get the total size.
        // Note: we also could use only one iteration and resize the vector size at each iteration.
        // -> however, computing first the full vector size allows to have only one vector memory allocation.
        size_t totalSize = size (x);

        DEBUG_SERIALIZATION ("[to  ] ++++++++++++++++++\n");

        // Second iteration: fill the buffer
        buffer_t buffer (totalSize);
        auto loop = buffer.data();
        iterate (0, x, [&] (int depth, void* ptr, size_t size, size_t roundedSize)
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
    static buffer_t tuple_to_buffer (const tuple<ARGS...>& x, INFO info, TRANSFO transfo, CALLBACK cbk)
    {
        // The serialized buffer is built by serializing each item of the provided tuple.
        // A previous version computed first the size of the buffer in order to allocate
        // the correct size. Now, we prefer to build the buffer dynamically and update the
        // buffer size when needed. Actually, we provided a initial 'defaultSize' size that
        // could be enough to cope with several small sizeof for the tuple items.
        size_t defaultSize = 256;

        // We create a buffer with a default size (may be extended if needed)
        buffer_t buffer (defaultSize);

        size_t idxArg       = 0;
        size_t offsetLocal  = 0;
        size_t offsetGlobal = 0;

        for_each_in_tuple (x, [&] (auto&& itemTuple)
        {
            DEBUG_SERIALIZATION ("[tuple_to_buffer  ]  ------------------------------------------> local: %ld  global: %ld\n", offsetLocal, offsetGlobal);

            // For each item of the tuple, we apply a transformation.
            // Such a transformation may for instance split the item in several parts.
            size_t idxTransfo = 0;

            auto fct = [&] (bool status, auto& item)
            {
                DEBUG_SERIALIZATION ("[tuple_to_buffer  ]      status: %d\n", status);

                // We serialize the item only if the status allows to do so.
                if (status)
                {
                    offsetLocal = 0;

                    iterate (0, item,  [&] (int depth, void* ptr, size_t size, size_t roundedSize)
                    {
                        // We may have to resize the buffer if needed. Note that we add a few extra 'defaultSize' bytes
                        if (offsetGlobal+roundedSize >= buffer.size())
                        {
                            buffer.resize (offsetGlobal + roundedSize + defaultSize);
                        }

                        // We can do a memcpy here since the data pointed by 'ptr' is supposed
                        // to be available at this time in memory. We are also sure about the size
                        // available in 'buffer' since its size has been previously updated if needed.
                        memcpy (buffer.data() + offsetGlobal, (char*)ptr, size);

                        offsetLocal  += roundedSize;
                        offsetGlobal += roundedSize;
                    });
                }

                // We call the callback in order to provide some information during the serialization process.
                // NOTE: we could have used a std::vector in order to gather this information and then provide it
                // to the caller. However, the serialize capacity we implement here can be used either by the multicore
                // arch and the UPMEM arch, so we prefer here make no use of explicit std class but nevertheless provide
                // some information through a callback.
                cbk (idxTransfo, idxArg, offsetGlobal-offsetLocal, offsetLocal);

                idxTransfo++;
            };

            // We get some information about the current item of the tuple.
            // In particular, we are interested to know whether it has to be split or not.
            auto [level, isSplit, nbUnits] = info (idxArg, itemTuple);

            // According to the split status, we don't have the same action.
            if (not isSplit)
            //if (not isSplit or (isSplit and level==1))
            {
                // No split -> we can use the same item.
                // For instance with Upmem arch, it means that the same information will be broadcasted to all DPUs.
                for (size_t i=0; i<nbUnits; i++)  {  fct (i==0, itemTuple);  }
            }
            else
            {
                // Split -> several items will be created (one per split part)
                for (const auto& [status,itemRef] : transfo(idxArg, itemTuple))
                {
                    fct (status,itemRef);
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

        // We need to resize the buffer with the correct size (we may have used too much room with the 'resize')
        buffer.resize (round(offsetGlobal));

        return buffer;
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
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#include <bpl/utils/splitter.hpp>

// We need to make SplitProxy a serializable -> just forward to the inner attribute '_t'
template<typename L, bpl::core::SplitKind K, typename TT>
struct bpl::core::serializable<details::SplitProxy<L,K,TT>>
{
    // we tell that our structure can be serialized
    static constexpr int value = true;

    template<class ARCH, class BUFITER, int ROUNDUP, typename T, typename FCT>
    static auto iterate (int depth, const T& t, FCT fct)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, t._t, fct);
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
struct bpl::core::serializable<bpl::core::Range>
{
    // we tell that our structure can be serialized
    static constexpr int value = true;

    template<class ARCH, class BUFITER, int ROUNDUP, typename T, typename FCT>
    static auto iterate (int depth, const T& t, FCT fct)
    {
        //printf ("Range::iterate:  %ld %ld \n", *t.begin(), *t.end  ());
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, *t.begin(), fct);
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, *t.end  (), fct);
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
struct bpl::core::serializable<bpl::core::RakeRange>
{
    // we tell that our structure can be serialized
    static constexpr int value = true;

    template<class ARCH, class BUFITER, int ROUNDUP, typename T, typename FCT>
    static auto iterate (int depth, const T& t, FCT fct)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, *t.begin(), fct);
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, *t.end  (), fct);
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1,  t.begin().modulo(), fct);
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
struct bpl::core::serializable<bpl::core::once<T>> : std::true_type
{
    template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE, typename FCT>
    static auto iterate (int depth, const TYPE& t, FCT fct)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::iterate (depth+1, *t, fct);
    }

    template<class ARCH, class BUFITER, int ROUNDUP, typename TYPE>
    static auto restore (BUFITER& it, TYPE& result)
    {
        Serialize<ARCH,BUFITER,ROUNDUP>::restore (it, *result);
    }
};


#endif // __BPL_UTILS_SERIALIZATION_HPP__
