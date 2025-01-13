////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

// Shortcut macro
#define NS0(ARCH,...)   typename ARCH::template factory<__VA_ARGS__>
#define NS(ARCH,...)  NS0(ARCH,__VA_ARGS__) ::template

/** We define a macro to be used as a helper in the definition of any "Task class".
 */
#define USING(ARCH,...) \
                                      using arch_t      = NS0(ARCH,__VA_ARGS__);                \
    template<typename A, typename B>  using pair        = NS (ARCH,__VA_ARGS__) pair<A,B>;      \
    template<class T, std::size_t N>  using array       = NS (ARCH,__VA_ARGS__) array<T,N>;     \
    template<class T>                 using vector      = NS (ARCH,__VA_ARGS__) vector<T>;      \
    template<class T>                 using span        = NS (ARCH,__VA_ARGS__) span<T>;        \
    template<class T>                 using vector_view = NS (ARCH,__VA_ARGS__) vector_view<T>; \
    template<class ...Ts>             using tuple       = NS (ARCH,__VA_ARGS__) tuple<Ts...>;   \
    template<class T>                 using lock_guard  = NS (ARCH,__VA_ARGS__) lock_guard<T>;  \
    template<class T>                 using once        = NS (ARCH,__VA_ARGS__) once<T>;        \
    template<class T>                 using global      = NS (ARCH,__VA_ARGS__) global<T>;      \
    template<class T>                 using glonce      = NS (ARCH,__VA_ARGS__) global<NS (ARCH,__VA_ARGS__)once<T>>;\
                                      using string      = NS0(ARCH,__VA_ARGS__)::string;        \
                                      using size_t      = NS0(ARCH,__VA_ARGS__)::size_t;        \
                                      using mutex       = NS0(ARCH,__VA_ARGS__)::mutex;         \
                                                                                                \
    template< std::size_t I, class... Types > static auto get(const tuple<Types...>& t ) noexcept  { return ARCH::template get<I>(t); } \
    template< std::size_t I, class... Types > static auto get(      tuple<Types...>& t ) noexcept  { return ARCH::template get<I>(t); } \
    template<typename ...Ts>                  static auto make_tuple (Ts... t)      { return ARCH::make_tuple(t...); }\
    template<typename A, typename B>          static auto make_pair  (A&& a, B&& b) { return ARCH::make_pair(a,b); }\
    template <class T>                        static auto swap (T& a, T& b)  { ARCH::template swap(a,b);  }


////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
