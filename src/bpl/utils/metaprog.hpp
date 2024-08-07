////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_METAPROG__
#define __BPL_METAPROG__

#include <tuple>
#include <cstddef>
#include <cstdint>
// FOR THE FUTURE:  #include <boost/hana.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// type list
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class...>  struct types  {  using type = types; };

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TUPLE
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <int N, typename... T>
struct tuple_element;

template <typename T0, typename... T>
struct tuple_element<0, T0, T...> {
    typedef T0 type;
};
template <int N, typename T0, typename... T>
struct tuple_element<N, T0, T...> {
    typedef typename tuple_element<N-1, T...>::type type;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION ARGUMENTS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class Sig> struct args;

template<class R, class...Args>
struct args<R(Args...)> : types<Args...>
{
    using Return = R;
    template<int N> using param = tuple_element<N,Args...>;
};

template<class R, class O, class...Args>
struct args<R(O::*)(Args...)> : types<Args...>
{
    using Return = R;
    template<int N> using param = tuple_element<N,Args...>;
};

template<class R, class O, class...Args>
struct args<R(O::*)(Args...) const> : types<Args...>
{
    using Return = R;
    template<int N> using param = tuple_element<N,Args...>;
};

template<class Sig> using args_t=typename args<Sig>::type;

template<class Sig,int N> using parameter_t = typename args<Sig>::template param<N>::type;
template<class Sig>       using return_t    = typename args<Sig>::Return;

//////////////////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename... Param> struct GetParamType
{
   using prototype_t = Ret(*)(Param...);
   using args_t      = std::tuple<Param...>;
   static const int args_size = sizeof...(Param);

   public:
       GetParamType (prototype_t);
};

//////////////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/47906426/type-of-member-functions-arguments

template<typename Result, typename ...Args>  struct FunctionSignatureParserBase
{
    using return_type = Result;
    using args_tuple  = std::tuple<Args...>;

    static constexpr std::size_t args_size = std::tuple_size_v<args_tuple>;

    template <size_t i> struct arg
    {
        typedef typename std::tuple_element<i, args_tuple>::type type;
    };

    template <size_t i>  using arg_t = typename arg<i>::type;
};

template<typename T>  struct FunctionSignatureParser;

template<typename ClassType, typename Result, typename...Args>
struct FunctionSignatureParser<Result(ClassType::*)(Args...)>
  : FunctionSignatureParserBase<Result,Args...>
{
};

template<typename ClassType, typename Result, typename...Args>
struct FunctionSignatureParser<Result(ClassType::*)(Args...) const>
  : FunctionSignatureParserBase<Result,Args...>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/34672441/stdis-base-of-for-template-classesv
template < template <typename...> class base,typename derived>
struct is_base_of_template_impl
{
    template<typename... Ts>
    static constexpr std::true_type  test(const base<Ts...> *);

    static constexpr std::false_type test(...);

    using type = decltype(test(std::declval<std::decay_t<derived>*>()));
};

template < template <typename...> class base,typename derived>
using is_base_of_template_t = typename is_base_of_template_impl<base,derived>::type;

template < template <typename...> class base,typename derived>
static constexpr bool is_base_of_template_v = is_base_of_template_t<base,derived>::value;

//////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct remove_first_type  {  using type = T;  };

template<typename T>
using remove_first_type_t = typename remove_first_type<T>::type;

template<typename T, typename... Ts>
struct remove_first_type<std::tuple<T, Ts...>>
{
    typedef std::tuple<Ts...> type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/62855836/convert-tuple-types-to-decay-types
template <typename ... Ts>
constexpr auto decay_types (std::tuple<Ts...> const &)
   -> std::tuple<std::remove_cv_t<std::remove_reference_t<Ts>>...>;

template <typename T>
using decay_tuple = decltype(decay_types(std::declval<T>()));


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/61112088/transform-tuple-type-to-another-tuple-type
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename Tuple, template <typename> typename... Component>
struct transform_tuple;

template <typename... Ts, template <typename> typename HEAD>
struct transform_tuple<std::tuple<Ts...>, HEAD>
{
    using type = std::tuple<typename HEAD<Ts>::type ...>;
};

template <typename... Ts, template <typename> typename HEAD, template <typename> typename... TAIL>
struct transform_tuple<std::tuple<Ts...>, HEAD, TAIL...>
{
    using head = typename transform_tuple <std::tuple<Ts...>, HEAD>:: type;
    using type = typename transform_tuple <head, TAIL...>::type ;
};

template<typename T, template <typename> typename... Component>
using transform_tuple_t = typename transform_tuple<T,Component...>::type;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T>
struct surrogate  { using type = T; };

template<typename T>
using surrogate_t = typename surrogate<T>::type;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/26902633/how-to-iterate-over-a-stdtuple-in-c-11
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class F, class...Ts, std::size_t...Is>
void for_each_in_tuple(const std::tuple<Ts...> & tuple, [[maybe_unused]] F func, std::index_sequence<Is...>)
{
    using expander = int[];
    (void)expander { 0, ((void)func(std::get<Is>(tuple)), 0)... };
}

template<class F, class...Ts>
void for_each_in_tuple(const std::tuple<Ts...> & tuple, F func)
{
    for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
}

// we also need a non const version in case we want to modify one item of the tuple during iteration
template<class F, class...Ts, std::size_t...Is>
void for_each_in_tuple(std::tuple<Ts...> & tuple, F func, std::index_sequence<Is...>)
{
    using expander = int[];
    (void)expander { 0, ((void)func(std::get<Is>(tuple)), 0)... };
}

template<class F, class...Ts>
void for_each_in_tuple(std::tuple<Ts...> & tuple, F func)
{
    for_each_in_tuple(tuple, func, std::make_index_sequence<sizeof...(Ts)>());
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// https://gist.github.com/utilForever/1a058050b8af3ef46b58bcfa01d5375d
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class T, typename... Args>  decltype(void(T{std::declval<Args>()...}), std::true_type())  test (int);
template<class T, typename... Args>  std::false_type                                               test (...);
template<class T, typename... Args> struct is_braces_constructible : decltype(test<T, Args...>(0))  {};


struct any_type {
  template<class T>
  constexpr operator T(); // non explicit
};

template<class T>
auto to_tuple(T&& object) noexcept
{
    using type = std::decay_t<T>;
    if constexpr(is_braces_constructible<type, any_type, any_type, any_type, any_type>{}) {
      auto&& [p1, p2, p3, p4] = object;
      return std::make_tuple(p1, p2, p3, p4);
    } else if constexpr(is_braces_constructible<type, any_type, any_type, any_type>{}) {
      auto&& [p1, p2, p3] = object;
      return std::make_tuple(p1, p2, p3);
    } else if constexpr(is_braces_constructible<type, any_type, any_type>{}) {
      auto&& [p1, p2] = object;
      return std::make_tuple(p1, p2);
    } else if constexpr(is_braces_constructible<type, any_type>{}) {
      auto&& [p1] = object;
      return std::make_tuple(p1);
    } else {
        return std::make_tuple();
    }
}

namespace details
{

template< typename result_type, typename ...types, std::size_t ...indices >
result_type
make_struct(std::tuple< types... > t, std::index_sequence< indices... >) // &, &&, const && etc.
{
    return {std::get< indices >(t)...};
}

}

template< typename result_type, typename ...types >
result_type
make_struct(std::tuple< types... > t) // &, &&, const && etc.
{
    return details::make_struct< result_type, types... >(t, std::index_sequence_for< types... >{}); // if there is repeated types, then the change for using std::index_sequence_for is trivial
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template<class TASK, bool DECAY=true>
using task_params_t = std::conditional_t<DECAY,
    bpl::core::decay_tuple <
        typename bpl::core::FunctionSignatureParser<decltype(&TASK::operator())>::args_tuple
    >,
    typename bpl::core::FunctionSignatureParser<decltype(&TASK::operator())>::args_tuple
>;

template<class TASK>
using task_params_nodecay_t = task_params_t<TASK,false>;


// see https://stackoverflow.com/questions/40536060/c-perfect-forward-copy-only-types-to-make-tuple
template<class TASK, typename ...ARGS>
auto  make_params (ARGS... args) {  return task_params_t<TASK> (std::forward<ARGS>(args)...); }

////////////////////////////////////////////////////////////////////////////////
template<int N>  int roundUp(int numToRound)  {  return (numToRound + N - 1) & -N; }

////////////////////////////////////////////////////////////////////////////////
template<typename T>  struct ClassName  { static constexpr char name[] = "?";    };

template<>  struct ClassName<int>    { static constexpr char name[] = "int";     };
template<>  struct ClassName<double> { static constexpr char name[] = "double";  };


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename T, int PADDING>
struct TypeWithPadding
{
    using type = union Dummy
    {
        Dummy (T&& o) : object(std::move(o)) {}

        T    object;
        char padding[PADDING];
    };
};

template<typename T, int PADDING=4>
using TypeWithPadding_t = typename TypeWithPadding<T,PADDING>::type;

template<typename ... input_t>
using tuple_cat_t= decltype(std::tuple_cat(std::declval<input_t>()...));

template<int PADDING, typename ...ARGS>
struct TuplePadding
{
};

template<int PADDING, typename T0>
struct TuplePadding<PADDING, std::tuple<T0>>
{
    using type = std::tuple<TypeWithPadding_t<T0,PADDING>>;
};

template<int PADDING, typename HEAD, typename ...ARGS>
struct TuplePadding<PADDING, std::tuple<HEAD, ARGS...> >
{
    using type = tuple_cat_t <
        std::tuple<TypeWithPadding_t<HEAD,PADDING>>,
        typename TuplePadding<PADDING,std::tuple<ARGS...>>::type
    >;
};

template<int PADDING, typename ...ARGS>
using TuplePadding_t = typename TuplePadding<PADDING,ARGS...>::type;


////////////////////////////////////////////////////////////////////////////////

template <int x>
struct Log2 { static const int value = 1 + Log2<x/2>::value ; };

template <> struct Log2<1> { static const int value = 1 ; };

////////////////////////////////////////////////////////////////////////////////
// Get the index of the type in a tuple that fulfills some predicate.
template<typename T, template<typename> class PREDICATE, bool CHECKALLFIRST>
struct GetIndexOfFirstPredicate
{
    static constexpr int value = 0;
};

template<typename T, template<typename> class PREDICATE, bool CHECKALLFIRST, typename ...ARGS>
struct GetIndexOfFirstPredicate<std::tuple<T,ARGS...>, PREDICATE, CHECKALLFIRST>
{
    // We make sure that all the references are the first types in the tuple, otherwise an assertion is set.
    static_assert (
        // the following condition is somehow easier than the simplified one (ie. remove double not):
        //  -> we have a failure if the current type doesn't match the predicate but the very next type in the tuple does.
        not (not PREDICATE<T>::value and  GetIndexOfFirstPredicate<std::tuple<ARGS...>,PREDICATE,CHECKALLFIRST>::value == 0)
        or
       (not CHECKALLFIRST)
        ,
        "All 'predicate' types should be the first ones of the tuple"
    );

    static constexpr int value =  PREDICATE<T>::value ?
        0 :
        1 + GetIndexOfFirstPredicate<std::tuple<ARGS...>,PREDICATE,CHECKALLFIRST>::value;
};

template<typename T, template<typename> class PREDICATE, bool CHECKALLFIRST>
struct GetIndexOfFirstPredicate<std::tuple<T>, PREDICATE, CHECKALLFIRST>
{
    static constexpr int value =  PREDICATE<T>::value ? 0 : 1;
};

// If there is only references, the returned value is the number of items of the tuple
template <typename _Tp, template<typename> class PREDICATE, bool CHECKALLFIRST=true>
    constexpr int first_predicate_idx_v = GetIndexOfFirstPredicate<_Tp,PREDICATE,CHECKALLFIRST>::value;

////////////////////////////////////////////////////////////////////////////////
template<template<typename> class TRAIT>
struct negation
{
    template<typename T>
    struct type : std::conditional_t <TRAIT<T>::value, std::false_type, std::true_type>  {};
};

template<template<typename> class TRAIT> using negation_t = typename negation<TRAIT>::type;

////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/8569567/get-part-of-stdtuple

namespace detail
{
    template <std::size_t Ofst, class Tuple, std::size_t... I>
    constexpr auto slice_impl(Tuple&& t, std::index_sequence<I...>)
    {
        return std::forward_as_tuple(
            std::get<I + Ofst>(std::forward<Tuple>(t))...);
    }
}

template <std::size_t I1, std::size_t I2, class Cont>
constexpr auto tuple_slice_aux(Cont&& t)
{
    static_assert(I2 >= I1, "invalid slice");
    static_assert(std::tuple_size<std::decay_t<Cont>>::value >= I2,
        "slice index out of bounds");

    return detail::slice_impl<I1>(std::forward<Cont>(t),
        std::make_index_sequence<I2 - I1>{});
}

template <std::size_t I1, std::size_t I2, class Cont>
constexpr auto tuple_slice(Cont&& t)
{
    if constexpr(I2 < I1)  { return std::tuple<> {}; }
    else                   { return tuple_slice_aux<I1,I2>(t);  }
}

//////////////////////////////////////////////////////////////////////////////////////////
template<template<typename> typename PREDICATE, typename ...ARGS>
struct CheckPredicate  {  const static int value = 0;  };

template<template<typename> typename PREDICATE, typename T, typename ...ARGS>
struct CheckPredicate<PREDICATE,T, ARGS...>
{
    const static int value = (PREDICATE<T>::value ? 1 : 0) + CheckPredicate<PREDICATE, ARGS...>::value;
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/25845536/trait-to-check-if-some-specialization-of-template-class-is-base-class-of-specifi
// http://coliru.stacked-crooked.com/a/9feadc62e7594eb2

template< template<typename ...formal > class base >
struct is_derived_from_impl
{
    template<typename ...actual >
    std::true_type
    operator () (base<actual... > *) const;

    std::false_type
    operator () (void *) const;
};

template< typename derived, template<typename ... > class base >
using is_derived_from = typename std::result_of< is_derived_from_impl< base >(typename std::remove_cv< derived >::type *) >::type;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<typename T, class BOUNDS>
struct slice
{
    const T& x_;
    BOUNDS bounds_;

    slice (const T& x, const BOUNDS& bounds)  : x_(x), bounds_(bounds) {}
    auto begin() const { return x_.begin() + bounds_.first ;  }
    auto end  () const { return x_.end  () + (bounds_.second - x_.size()); }
};

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template <class Range, class T> T accumulate (Range&& r, T init)
{
    auto first = r.begin();
    auto last  = r.end();
    while (first!=last)
    {
        init = init + *first;
        ++first;
    }
  return init;
}

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/13830158/check-if-a-variable-type-is-iterable
template<typename C>
struct is_iterable
{
  typedef long false_type;
  typedef char true_type;

  template<class T> static false_type check(...);
  template<class T> static true_type  check(int,
                    typename T::const_iterator = C().end());

  enum { value = sizeof(check<C>(0)) == sizeof(true_type) };
};

template <typename T>  constexpr int is_iterable_v = is_iterable<T>::value;

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////

template<typename T>
struct serializable  : std::false_type {};

template<typename T> static constexpr bool is_serializable_v =
    serializable<std::remove_cv_t<std::remove_reference_t<T>>>::value;

////////////////////////////////////////////////////////////////////////////////

template<template<typename> typename FCT, typename ...ARGS>
struct Predicate
{
    static constexpr bool value = true;
};

template<template<typename> typename FCT>
struct Predicate<FCT>
{
    static constexpr bool value = true;
};

template<template<typename> typename FCT, typename HEAD, typename...TAIL>
struct Predicate<FCT, HEAD, TAIL...>
{
    static constexpr bool value = FCT<HEAD>::value and Predicate<FCT,TAIL...>::value;
};

template<template<typename> typename FCT, typename ...ARGS>
static constexpr bool Predicate_v = Predicate<FCT,ARGS...>::value;

////////////////////////////////////////////////////////////////////////////////

/** Utility that returns a tuple from packed arguments.
 *   -> use either tie or make_tuple according to a template parameter.
 *   \param[in] args : the packed arguments
 *   \return the tuple
 */
template<typename ...T>
struct TieOrMaketuple
{
    template<typename ...ARGS>
    static auto get (ARGS&&... args)
    {
        // We check whether all the T types are lvalue references
        if constexpr(Predicate_v<std::is_lvalue_reference, T...>)
        {
            return std::tie (args...);
        }
        else
        {
            return std::make_tuple (args...);
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

template<template <typename> class PREDICATE, typename ...ARGS>
struct pack_create_mask  {};

template<template <typename> class PREDICATE, typename ...ARGS>
static constexpr unsigned long long pack_create_mask_v = pack_create_mask<PREDICATE,ARGS...>::value;

template<template <typename> class PREDICATE, typename T, typename ...ARGS>
struct pack_create_mask<PREDICATE,T,ARGS...>
{
  static constexpr unsigned long long value =
    PREDICATE<T>::value |
    (pack_create_mask<PREDICATE,ARGS...>::value) << 1;
};

template<template <typename> class PREDICATE>
struct pack_create_mask<PREDICATE>
{
  static constexpr unsigned long long value = 0;
};

////////////////////////////////////////////////////////////////////////////////

template<template <typename> class PREDICATE, typename ...ARGS>
struct tuple_create_mask  {};

template<template <typename> class PREDICATE, typename T, typename ...ARGS>
struct tuple_create_mask<PREDICATE,std::tuple<T, ARGS...>>
{
  static constexpr unsigned long long value =
      PREDICATE<T>::value |
      (tuple_create_mask<PREDICATE,std::tuple<ARGS...>>::value) << 1;
};

template<template <typename> class PREDICATE>
struct tuple_create_mask<PREDICATE, std::tuple<> >
{
  static constexpr unsigned long long value = 0;
};

template<template <typename> class PREDICATE, typename ...ARGS>
static constexpr unsigned long long tuple_create_mask_v = tuple_create_mask<PREDICATE,ARGS...>::value;

///////////////////////////////////////////////////////////////////////////////////////////

template<unsigned long long MASK, typename ...ARGS>
struct pack_filter_by_mask {  };

template<unsigned long long MASK, typename ...ARGS>
using pack_filter_by_mask_t = typename pack_filter_by_mask<MASK,ARGS...>::type;

template<unsigned long long MASK, typename T, typename ...ARGS>
struct pack_filter_by_mask<MASK,std::tuple<T,ARGS...>>
{
    // Just for readability
    using TAIL = pack_filter_by_mask_t <(MASK>>1),std::tuple<ARGS...> >;

    using type = std::conditional_t <
        MASK&1,
        decltype(std::tuple_cat (
          std::make_tuple (std::declval<T>()),
          std::declval<TAIL>()
        )),
        TAIL
    >;
};

template<unsigned long long MASK>
struct pack_filter_by_mask<MASK,std::tuple<>>  {  using type = std::tuple<>;  };

///////////////////////////////////////////////////////////////////////////////////////////

template<unsigned long long MASK, typename ...ARGS>
struct pack_filter_by_mask_partition
{
  using type = std::pair <
    pack_filter_by_mask_t< MASK,ARGS...>,
    pack_filter_by_mask_t<~MASK,ARGS...>
  >;
};

template<unsigned long long MASK, typename ...ARGS>
using pack_filter_by_mask_partition_t = typename pack_filter_by_mask_partition <MASK,ARGS...>::type;

template<template<typename> class PREDICATE, typename ...ARGS>
using pack_predicate_partition_t =
    pack_filter_by_mask_partition_t <
        tuple_create_mask<PREDICATE,ARGS...>::value,
        ARGS...
    >;

///////////////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/12666761/sizeof-variadic-template-sum-of-sizeof-of-all-elements
template<typename...ARGS>
inline constexpr unsigned int sum_sizeof (const std::tuple<ARGS...>& t)  {  return (0 + ... + sizeof(ARGS));  }

///////////////////////////////////////////////////////////////////////////////////////////

// https://stackoverflow.com/questions/78544926/merging-two-tuples-according-to-a-given-criteria/78545217#78545217

template <std::size_t N>
constexpr std::array<std::pair<std::size_t, std::size_t>, N>
mask_to_array(std::uint64_t mask)
{
    std::array<std::pair<std::size_t, std::size_t>, N> res{};
    std::size_t ai = 0;
    std::size_t bi = 0;

    for (auto& [ab, i] : res) {
        ab = mask & 1;
        i = ab ? ai++ : bi++;
        mask >>= 1;
    }
    return res;
}

template <std::uint64_t MASK, typename...A, typename...B>
constexpr auto merge_tuples (std::tuple<A...>& a, std::tuple<B...>& b)
{
    constexpr std::size_t N = sizeof...(A) + sizeof...(B);
    constexpr auto arr = mask_to_array<N>(MASK);

    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::tie(std::get<arr[Is].second>(std::get<arr[Is].first>(std::tie(b, a)))...);

    }(std::make_index_sequence<N>());
}

///////////////////////////////////////////////////////////////////////////////////////////
struct identity
{
    template<typename U>  constexpr U&& operator() (U&& u) const noexcept { return std::forward(u); }
};

struct pointer
{
    template<typename U>  constexpr U* operator()  (U&& u) const noexcept { return &u; }
};

///////////////////////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/78116330/compilation-time-initialization-of-an-array-of-struct-with-the-information-of-an
template<typename T, int N>
struct array_wrapper
{
    using type = T;
    static constexpr int size = N;

    T value[N];

    T& operator[] (std::size_t n)  { return value[n]; }

    template <typename U>
    static constexpr auto init_from (U source[N])
    {
        return [&]<std::size_t...Is>(std::index_sequence<Is...>)
        {
            return  bpl::core::array_wrapper<T,N> {{ T (source[Is] )... }};
        }
        (std::make_index_sequence<N>());
    }
};

// We need a template specialization for 0-size array
template<typename T>
struct array_wrapper<T,0>
{
    template <typename U>
    static constexpr auto init_from (U* source) { return array_wrapper<T,0>(); }
};

////////////////////////////////////////////////////////////////////////////////
// https://codereview.stackexchange.com/questions/193420/apply-a-function-to-each-element-of-a-tuple-map-a-tuple

template <class F, typename Tuple, size_t... Is>
auto transform_each_impl (Tuple t, F f, std::index_sequence<Is...>)
{
    return std::make_tuple(f(std::get<Is>(t) )...);
}

template <class F, typename... Args>
auto transform_each(const std::tuple<Args...>& t, F f)
{
    return transform_each_impl(t, f, std::make_index_sequence<sizeof...(Args)>{});
}

////////////////////////////////////////////////////////////////////////////////

template<typename TIter1, typename TIter2>
struct zip_iterator
{
    TIter1 iter1_;
    TIter2 iter2_;

    template<typename OTHER>
    bool operator != (const OTHER& other) const
    {
        return (iter1_ != other.iter1_) and (iter2_ != other.iter2_);
    }

    void operator++ ()  {  ++iter1_;  ++ iter2_;  }

    auto operator * () const
    {
        return TieOrMaketuple <decltype(*iter1_), decltype(*iter2_)>::get (*iter1_,*iter2_);
    }
};


template <typename T1,
          typename T2,
          typename TIter1 = decltype(std::declval<T1>().begin()),
          typename TIter2 = decltype(std::declval<T2>().begin())
>
constexpr auto zip (T1&&  iterable1, T2&&  iterable2)
{
//    struct iterator
//    {
//        TIter1 iter1_;
//        TIter2 iter2_;
//
//        bool operator != (const iterator & other) const
//        {
//            return (iter1_ != other.iter1_) and (iter2_ != other.iter2_);
//        }
//
//        void operator++ ()  {  ++iter1_;  ++ iter2_;  }
//
//        auto operator * () const
//        {
//            return TieOrMaketuple <decltype(*iter1_), decltype(*iter2_)>::get (*iter1_,*iter2_);
//        }
//    };
    struct iterable_wrapper
    {
        T1 iterable1_;
        T2 iterable2_;

        auto begin() const { return zip_iterator<decltype(iterable1_.begin()),decltype(iterable2_.begin())>{ iterable1_.begin(), iterable2_.begin() }; }
        auto end()   const { return zip_iterator<decltype(iterable1_.end()),decltype(iterable2_.end())> { iterable1_.end(),   iterable2_.end  () }; }
    };

    return iterable_wrapper { std::forward<T1>(iterable1) , std::forward<T2>(iterable2)  };
}

////////////////////////////////////////////////////////////////////////////////

template <template<typename,typename> class COMPARATOR, typename A, typename B>
struct compare_tuples : std::true_type {};

template <template<typename,typename> class COMPARATOR, typename A, typename B>
static constexpr bool compare_tuples_v = compare_tuples<COMPARATOR,A,B>::value;

template <template<typename,typename> class COMPARATOR>
struct compare_tuples<COMPARATOR,std::tuple<>, std::tuple<>> : std::true_type {};

template <template<typename,typename> class COMPARATOR, typename...A, typename... B>
struct compare_tuples<COMPARATOR,std::tuple<A...>, std::tuple<B...>>
{
    static_assert (sizeof...(A) == sizeof...(B));

    using TA = std::tuple<A...>;
    using TB = std::tuple<B...>;

    static constexpr bool value =
        COMPARATOR<std::tuple_element<0,TA>,std::tuple_element<0,TB>>::value &&
            compare_tuples_v<COMPARATOR,remove_first_type_t<TA>,remove_first_type_t<TB>>;
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif // __BPL_METAPROG__
