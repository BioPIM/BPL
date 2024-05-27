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

// Maybe we could do better than a big macro... (perhaps with CRTP)

#define TAG_DEFINITION(NAME)                                \
    template<typename T>                                    \
    struct NAME                                             \
    {                                                       \
        using type  = T;                                    \
        using ptr   = type*;                                \
        using ref   = type&;                                \
                                                            \
        NAME()             = default;                       \
        NAME(const NAME&)  = default;                       \
        NAME(      NAME&&) = default;                       \
                                                            \
        NAME& operator= (const NAME&)  = default;           \
        NAME& operator= (      NAME&&) = default;           \
                                                            \
        template<typename U>                                \
        NAME(U&& t)  : ptr_(&t) {}                          \
                                                            \
        ptr  operator-> () const { return  ptr_; }          \
        ref  operator*  () const { return *ptr_; }          \
                                                            \
        ptr  operator-> ()       { return  ptr_; }          \
        ref  operator*  ()       { return *ptr_; }          \
                                                            \
        ptr ptr_;  /* keep the reference as a pointer. */   \
    };                                                      \
                                                            \
    template<typename T>                                    \
    struct hastag_##NAME            : std::false_type {};   \
                                                            \
    template<typename T>                                    \
    struct hastag_##NAME<NAME<T>>   : std::true_type {};    \
                                                            \
    template<typename T>                                    \
    static constexpr bool hastag_##NAME##_v =               \
           hastag_##NAME<T>::value;                         \
                                                            \
    template<typename T>                                    \
    struct hasnotag_##NAME            : std::true_type {};  \
                                                            \
    template<typename T>                                    \
    struct hasnotag_##NAME<NAME<T>>   : std::false_type {}; \
                                                            \
    template<typename T>                                    \
    static constexpr bool hasnotag_##NAME##_v =             \
           hasnotag_##NAME<T>::value;                       \
                                                            \
    template<typename T>                                    \
    struct removetag_##NAME           { using type = T; };  \
                                                            \
    template<typename T>                                    \
    struct removetag_##NAME<NAME<T>>  { using type = T; };

//////////////////////////////////////////////////////////////////////////////////////////
TAG_DEFINITION(once);
TAG_DEFINITION(global);

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

