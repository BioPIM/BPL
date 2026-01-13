////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/utils/metaprog.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl {
////////////////////////////////////////////////////////////////////////////////

    // Enumeration holding different parallelization schemes.
    // NOTE: not used right now (only CONT is relevant)
    enum SplitKind
    {
        CONT = 0,   // split by contiguous chunks; ex: [0:5]  => [0,1,2] [2,3,4]
        RAKE = 1,   // interleave split;           ex: [0:5]  => [0,2,4] [1,3,5]
        RAND = 2    // random split                ex: [0:5]  => [0,3,5] [1,2,4]
    };

namespace impl
{
    // We define the notion of parallelization level that gathers:
    //    * a const integer
    //    * an underlying architecture

    // We define by default a dummy level.
    struct DummyLevel
    {
        constexpr static int LEVEL = -1;
        using arch_t = void;
    };

    /** Structure that encapsulates a function argument of type T.
     * \param L    : type holding the level
     * \param KIND : a split scheme
     * \param TYPE : the type of the proxied object
     */
    template<typename L, bpl::SplitKind KIND, typename TYPE>
    struct SplitProxy
    {
        constexpr static int LEVEL = L::LEVEL;
        static_assert (LEVEL>=-1 and LEVEL<=3);

        // we may need to decay the incoming type T
        using type   = std::decay_t<TYPE>;

        // an alias for the underlying architecture
        using arch_t = typename L::arch_t;

        /** Constructor
         * \param t : the object we want to keep a reference on.
         */
        SplitProxy (const type& t) : _t(t)  {}

        /** Cast operator
         * \return the reference on the proxied object
         */
        operator const type& () const { return _t; }

        /** reference on the object provided through the constructor.
         *    => we must be sure that this object lives while using a proxy on it. */
        const type& _t;
    };

    ////////////////////////////////////////////////////////////////////////////////
    /** Type trait that returns the parallelization level from a type.
     * This default implementation returns 0.
     */
    template<typename T, typename DEFAULT=impl::DummyLevel>
    struct GetSplitStatus
    {
        static const int value = 0;
    };

    /** Type trait specialization for the SplitProxy type.
     * The value is then:
     *   - the level of the SplitProxy in case this is not the dummy level
     *   - the default provided level
     */
    template<typename L, bpl::SplitKind K, typename T,typename DEFAULT>
    struct GetSplitStatus<SplitProxy<L,K,T>,DEFAULT>
    {
        static const int value = SplitProxy<L,K,T>::LEVEL == impl::DummyLevel::LEVEL ?
            DEFAULT::LEVEL :
            SplitProxy<L,K,T>::LEVEL;
    };
}

////////////////////////////////////////////////////////////////////////////////

// We define a class intended to be specialized, in particular with a 'split' method telling how to split T.
// Such specializations are provided for types such as 'vector' for instance (either for std nor bpl).
template <typename T>  struct SplitOperator  {};

// A type is 'splitable' if it provides a 'split' method
template<typename T>
static constexpr bool is_splitable_v = requires(const T& t, size_t idx, size_t total ) { SplitOperator<std::decay_t<T>>::split(t, idx, total); };

template<typename T, typename TASK>
static constexpr bool is_custom_splitable_v = requires(const T& t, size_t idx, size_t total ) { TASK::split(t, idx, total); };

template<typename T,typename TASK=void>
requires (is_splitable_v<T>)
auto split (const T& t, size_t idx, size_t total)  {  return SplitOperator<T>::split (t,idx,total);  }

template<typename T,typename TASK>
requires (is_custom_splitable_v<T,TASK>)
auto split (const T& t, size_t idx, size_t total)  {  return TASK::split (t,idx,total); }

////////////////////////////////////////////////////////////////////////////////
template<typename T, typename TASK>
auto split_assign (T& t, size_t idx, size_t total)  {}

template<typename T, typename TASK>
requires (is_splitable_v<T>)
auto split_assign (T& t, size_t idx, size_t total)  {  t = split<T> (t, idx, total);  }

template<typename T, typename TASK>
requires (is_custom_splitable_v<T, TASK>)
auto split_assign (T& t, size_t idx, size_t total)  {  t = split<T,TASK> (t,idx,total); }

////////////////////////////////////////////////////////////////////////////////

// This class can choose the correct 'split' function to be called according to the provided
// template type information:
//
template<typename T, typename RESULT, typename TASK>
struct SplitChoice {};

template<typename T, typename RESULT, typename TASK>
inline constexpr bool has_split_view_v = requires(T t, size_t idx, size_t total) { SplitOperator<RESULT>::split_view (t,idx,total); };

template<typename T, typename RESULT, typename TASK>
requires (has_split_view_v<T,RESULT,TASK>)
struct SplitChoice<T,RESULT,TASK>
{
    static decltype(auto) split_view (const T& t, size_t idx, size_t total)  {  return SplitOperator<RESULT>::split_view (t, idx, total); }
    static decltype(auto) split      (const T& t, size_t idx, size_t total)  {  return SplitOperator<RESULT>::split      (t, idx, total); }
};

template<typename T, typename RESULT, typename TASK>
inline constexpr bool has_split_request_v = requires(T t, size_t idx, size_t total) { TASK::split (t,idx,total); };

template<typename T, typename RESULT, typename TASK>
requires (has_split_request_v<T,RESULT,TASK>)
struct SplitChoice<T,RESULT,TASK>
{
    static decltype(auto) split (const T& t, size_t idx, size_t total)  {  return TASK::split(t, idx, total); }
    static decltype(auto) split_view (const T& t, size_t idx, size_t total)  {  return TASK::split_view(t, idx, total); }
};

////////////////////////////////////////////////////////////////////////////////

/** Method that encapsulates an incoming object of type T.
 * \param LEVEL : the level type
 * \param TYPE  : the type of the object
 * \return a SplitProxy instance that proxies the incoming object
 */
template<typename LEVEL, typename TYPE>
//requires (is_splitable_v<TYPE>)
auto split (TYPE& t)
{
    return impl::SplitProxy<LEVEL,bpl::SplitKind::CONT,TYPE> (t);
}

/** Method that encapsulates an incoming object of type T.
 * \param LEVEL : the level type
 * \param TYPE  : the type of the object
 * \return a SplitProxy instance that proxies the incoming object
 */
template<typename LEVEL, typename TYPE>
//requires (is_splitable_v<TYPE>)
auto  split (const TYPE& t)
{
    return impl::SplitProxy<LEVEL,bpl::SplitKind::CONT,TYPE> (t);
}

////////////////////////////////////////////////////////////////////////////////
/** Method that encapsulates an incoming object of type T.
 * \param TYPE  : the type of the object
 * \return a SplitProxy instance that proxies the incoming object
 */
template<typename TYPE>
//requires (is_splitable_v<TYPE>)
auto  split (TYPE& t)
{
    return split<impl::DummyLevel,TYPE> (t);
}

/** Method that encapsulates an incoming object of type T.
 * \param TYPE  : the type of the object
 * \return a SplitProxy instance that proxies the incoming object
 */
template<typename TYPE>
//requires (is_splitable_v<TYPE>)
auto  split (const TYPE& t)
{
    return split<impl::DummyLevel,TYPE> (t);
}

////////////////////////////////////////////////////////////////////////////////

/** Type trait providing a boolean telling whether or not the provided type is a SplitProxy instance
 * \param T : the type of the type trait
 */
template<typename T>  struct is_splitter : std::false_type {};

/** Type trait specialization in case the incoming type is a SplitProxy  */
template<typename L, bpl::SplitKind K, typename T>
struct is_splitter<impl::SplitProxy<L,K,T>> : std::true_type {};

/** Type trait value's getter.  */
template<typename T>  inline constexpr bool is_splitter_v = is_splitter<T>::value;

////////////////////////////////////////////////////////////////////////////////

/** Type trait that retrieves the type of the proxy object in case the type is a SplitProxy
 */
template<typename T>  struct remove_splitter {  using type = T;  };

/** Type trait specialization in case the incoming type is a SplitProxy  */
template<typename L, bpl::SplitKind K, typename T>
struct remove_splitter<impl::SplitProxy<L,K,T>> {  using type = T;  };

/** Type trait value's getter.  */
template<typename T>  using remove_splitter_t = typename remove_splitter<T>::type;

template <typename ... Ts>
constexpr auto decay_splitter (std::tuple<Ts...> const &)  -> std::tuple < remove_splitter_t<Ts> ...>;

/** Type trait that calls 'remove_splitter' on all items of a tuple..  */
template <typename T>
using decay_splitter_tuple = decltype(decay_splitter(std::declval<T>()));

////////////////////////////////////////////////////////////////////////////////

/** Compute the split status of a parameters pack as an array of integer values.
 * \param status : array (size 32) to be filled with the split status
 */
template<typename DEFAULT, typename ...ARGS>
void retrieveSplitStatus (uint8_t status[32])
{
    //   0: no split
    //   1: split at rank level
    //   2: split at DPU level
    //   3: split at tasklet level
    static_assert (sizeof...(ARGS)<=32);

    for (int i=0; i<32; i++)  { status[i]=0; }

    // https://stackoverflow.com/questions/59052742/how-to-get-index-from-fold-expression
    int i=0;   ( (status[i] = impl::GetSplitStatus<ARGS,DEFAULT>::value, i++), ...);
}

////////////////////////////////////////////////////////////////////////////////
/** Type trait specialization in case the incoming type is a SplitProxy */
template<typename LEVEL, bpl::SplitKind KIND, typename TYPE>
struct SplitOperator<impl::SplitProxy<LEVEL,KIND,TYPE>>
{
    /** Compute the ith partition of a given object
     * \param t : the object we want to compute the partition from
     * \param idx : the index of the partition to be computed
     * \param total : total number of partitions
     * \return the ith partition
     */
    static decltype(auto) split (const impl::SplitProxy<LEVEL,KIND,TYPE>& t, std::size_t idx, std::size_t total)
    {
        // We simply forward to the encapsulated object.
        return SplitOperator<TYPE>::split (t, idx, total);
    }
};

////////////////////////////////////////////////////////////////////////////////
}
////////////////////////////////////////////////////////////////////////////////
