////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <type_traits>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

// We define a type trait that allows to convert a type in case it is used in a 'global' context.
// Specialization can be written when needed; for instance, we may want to have an external cache
// for vector_view iterators (ie. not reuse the shared cache for iteration in a multi-tasklets context)
template<typename T>  struct global_converter  {  using type = T; };
template<typename T>   using global_converter_t = typename global_converter<T>::type;

// Maybe we could do better than a big macro... (perhaps with CRTP)
#define TAG_DEFINITION(NAME)                                    \
    template<typename T>                                        \
    struct NAME                                                 \
    {                                                           \
        using input = std::remove_reference_t<std::decay_t<T>>; \
                                                                \
        using type = global_converter_t<input>;                 \
        using ptr  = type*;                                     \
        using ref  = type&;                                     \
                                                                \
        NAME()             = default;                           \
        NAME(const NAME&)  = default;                           \
        NAME(      NAME&&) = default;                           \
                                                                \
        NAME& operator= (const NAME&)  = default;               \
        NAME& operator= (      NAME&&) = default;               \
                                                                \
        template<typename U>                                    \
        NAME(U&& u)  : ptr_(&u) {}                              \
                                                                \
        ptr  operator-> () const { return  ptr_; }              \
        ref  operator*  () const { return *ptr_; }              \
                                                                \
        ptr  operator-> ()       { return  ptr_; }              \
        ref  operator*  ()       { return *ptr_; }              \
                                                                \
        ptr ptr_;  /* keep the reference as a pointer. */       \
    };                                                          \
                                                                \
    template<typename T>                                        \
    struct hastag_##NAME            : std::false_type {};       \
                                                                \
    template<typename T>                                        \
    struct hastag_##NAME<NAME<T>>   : std::true_type {};        \
                                                                \
    template<typename T>                                        \
    static constexpr bool hastag_##NAME##_v =                   \
           hastag_##NAME<T>::value;                             \
                                                                \
    template<typename T>                                        \
    struct hasnotag_##NAME            : std::true_type {};      \
                                                                \
    template<typename T>                                        \
    struct hasnotag_##NAME<NAME<T>>   : std::false_type {};     \
                                                                \
    template<typename T>                                        \
    static constexpr bool hasnotag_##NAME##_v =                 \
           hasnotag_##NAME<T>::value;                           \
                                                                \
    template<typename T>                                        \
    struct removetag_##NAME           { using type = T; };      \
                                                                \
    template<typename T>                                        \
    struct removetag_##NAME<NAME<T>>  { using type = typename NAME<T>::type; };

//////////////////////////////////////////////////////////////////////////////////////////
TAG_DEFINITION(once);
TAG_DEFINITION(global);

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

