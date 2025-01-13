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

// We define a type trait that allows to convert a type in case it is used in a 'global' context.
// Specialization can be written when needed; for instance, we may want to have an external cache
// for vector_view iterators (ie. not reuse the shared cache for iteration in a multi-tasklets context)
template<typename T>  struct global_converter  {  using type = T; };

// We define a converter that does nothing but returning the T type.
template<typename T>  struct null_converter    {  using type = T; };

// General definition of a tag
template<typename T>
struct tag
{
    using type = T;
    using ptr  = type*;
    using ref  = type&;

    constexpr tag() : ptr_(nullptr) {}

    template<typename U>
    tag (U&& u)  : ptr_(std::addressof(u)) {}  // don't use &u since the unary operator& may be deleted.

    ptr  operator-> () const { return  ptr_; }
    ref  operator*  () const { return *ptr_; }

    ptr  operator-> ()       { return  ptr_; }
    ref  operator*  ()       { return *ptr_; }

    ptr ptr_;  /* keep the reference as a pointer. */
};

////////////////////////////////////////////////////////////////////////////////////////////////////
// T may still refer a chain of tag (e.g. global<int>), so we have to go down until we find the first non 'tag' type (e.g. int)
//
template<typename T> struct first_non_tag  {  using type = T; };

template<typename T> using first_non_tag_t = typename first_non_tag<T>::type;

template<template<typename> class TAG, typename T>
requires bpl::is_base_of_template_v <tag, TAG<char>>
struct first_non_tag<TAG<T>> : first_non_tag<T> {};


////////////////////////////////////////////////////////////////////////////////////////////////////
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
// General definition of our type trait
template<template<typename> class TAG, typename T>
struct hastag  : std::false_type {};

// and some specializations
template<template<typename> class TAG, typename T>
requires bpl::is_base_of_template_v <tag, TAG<char>> and  bpl::is_base_of_template_v <TAG,T>
struct hastag<TAG, T> : std::true_type {};

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

