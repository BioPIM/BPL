////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <type_traits>
#include <bpl/utils/metaprog.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** \brief Type trait that allows to convert a type in case it is used in a 'global' context.
 *
 Specialization can be written when needed; for instance, we may want to have an external cache
 for vector_view iterators (ie. not reuse the shared cache for iteration in a multi-tasklets context)
 */
template<typename T>  struct global_converter  {  using type = T; };

/** \brief Converter that does nothing but returning the T type. */
template<typename T>  struct null_converter    {  using type = T; };

/** \brief Definition of a tag for a given type T.
 *
 * 'tag' allows to encapsulate a type T in order to associate some semantics to T.
 * It doesn't add behavior to T but merely keeps a reference to T object.
 *
 * This class is intended to be inherited from by actual tags, whose type name can
 * be used at compile time for adding some semantics.
 */
template<typename T>
struct tag
{
    using type = T;

    template<typename U>
    tag (U&& u)  : ref_(u) {}

    decltype(auto) operator-> () const { return  std::addressof(ref_); }
    decltype(auto) operator*  () const { return  ref_;  }

    decltype(auto) operator-> ()       { return  std::addressof(ref_); }
    decltype(auto) operator*  ()       { return  ref_;  }

    const T& ref_;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/** \brief Trait that retrieves the first non tagged type from T.
 *
 * T may still refer a chain of tag (e.g. global<int>), so we have to go down until we find the first
 * non 'tag' type (e.g. int)
 */
template<typename T> struct first_non_tag  {  using type = T; };

template<typename T> using first_non_tag_t = typename first_non_tag<T>::type;

/** \brief Template specialization for a type that inherits from 'tag'. */
template<template<typename> class TAG, typename T>
requires bpl::is_base_of_template_v <tag, TAG<char>>
struct first_non_tag<TAG<T>> : first_non_tag<T> {};

////////////////////////////////////////////////////////////////////////////////////////////////////
/** Macro that defines a tag, given its name and a potential converter (ie. a trait that can modify
 * the first non tagged type).
 */
#define TAG_DEFINITION(NAME,CONVERTER)                          \
                                                                \
  template<typename T>                                          \
  using NAME##_type = typename CONVERTER<                       \
    std::decay_t<first_non_tag_t<T>>>::type;                    \
                                                                \
  template<typename T>                                          \
    struct NAME : tag<NAME##_type<T>>                           \
    {                                                           \
        NAME()             = default;                           \
        NAME(const NAME&)  = default;                           \
        NAME(      NAME&&) = default;                           \
                                                                \
        NAME& operator= (const NAME&)  = default;               \
        NAME& operator= (      NAME&&) = default;               \
                                                                \
        template<typename U>                                    \
        NAME(U&& u) : tag<NAME##_type<T>> (u) {}                \
    };                                                          \
                                                                \
template<typename T>                                            \
using hastag_##NAME = hastag<NAME,T>;                           \
                                                                \
template<typename T>                                            \
inline constexpr bool hastag_##NAME##_v = hastag_v<NAME,T>;     \
                                                                \
template<typename T>                                            \
using hasnotag_##NAME = bpl::static_not<hastag<NAME,T>>;  \
                                                                \
template<typename T>                                            \
inline constexpr bool hasnotag_##NAME##_v = ! hastag_v<NAME,T>; \
                                                                \
template<typename T>                                            \
struct removetag_##NAME           { using type = T; };          \
                                                                \
template<typename T>                                            \
struct removetag_##NAME<NAME<T>>  { using type = typename NAME<T>::type; }

////////////////////////////////////////////////////////////////////////////////////////////////////
/** \brief Type traits telling whether a type T is encapsulated by a 'tag' class. */
template<template<typename> class TAG, typename T>
struct hastag  : std::false_type {};

/** \brief Template specialization.*/
template<template<typename> class TAG, typename T>
requires bpl::is_base_of_template_v <tag, TAG<char>> and  bpl::is_base_of_template_v <TAG,T>
struct hastag<TAG, T> : std::true_type {};

/** \brief Template specialization.*/
template<template<typename> class TAG1, template<typename> class TAG2, typename T>
requires (not std::is_same_v<TAG1<char>,TAG2<char>>)
struct hastag<TAG1, TAG2<T>> : hastag<TAG1,T> {};

template<template<typename> class TAG, typename T>
inline constexpr bool hastag_v = hastag<TAG,T>::value;

//////////////////////////////////////////////////////////////////////////////////////////
TAG_DEFINITION (once,     null_converter);
TAG_DEFINITION (global, global_converter);

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////

