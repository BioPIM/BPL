////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_ARCH__
#define __BPL_ARCH__
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace arch {
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

// Shortcut macro
#define NS(ARCH)  typename ARCH::template

/** We define a macro to be used as a helper in the definition of any "Task class".
 */
#define USING(ARCH) \
    template<typename A, typename B>  using pair   = NS(ARCH) pair<A,B>;    \
    template<class T, std::size_t N>  using array  = NS(ARCH) array<T,N>;   \
    template<class T>                 using vector = NS(ARCH) vector<T>;    \
    template<class T>                 using span   = NS(ARCH) span<T>; \
    template<class T>                 using vector_view = NS(ARCH) vector_view<T>;    \
    template<class ...Ts>             using tuple  = NS(ARCH) tuple<Ts...>; \
                                      using string = typename ARCH::string; \
                                      using size_t = typename ARCH::size_t;  \
    template<typename ...Ts> static auto make_tuple (Ts... t) { return ARCH::make_tuple(t...); } \
    template< std::size_t I, class... Types > static auto get(const tuple<Types...>& t ) noexcept  { return ARCH::template get<I>(t); } \
    template< std::size_t I, class... Types > static auto get(      tuple<Types...>& t ) noexcept  { return ARCH::template get<I>(t); } \
    using mutex = typename ARCH::mutex; \
    template<class T>                 using lock_guard = NS(ARCH) lock_guard<T>; \
    template <class T> static void swap (T& a, T& b)  { ARCH::template swap(a,b);  } \
    template<class T> using once   = NS(ARCH) once<T>; \
    template<class T> using global = NS(ARCH) global<T>;


////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
#endif // __BPL_ARCH__
