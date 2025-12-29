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
#define NS(ARCH,...)    NS0(ARCH,__VA_ARGS__) ::template
#define NSC(ARCH,...)   ARCH::template factory<__VA_ARGS__>::template

/** We define a macro to be used as a helper in the definition of any "Task class".
 */
#define USING(ARCH,...) \
                                      using arch_t      = NS0(ARCH,__VA_ARGS__);                \
                                      using traits_t    = std::tuple<__VA_ARGS__>;        \
    template<typename A, typename B>  using pair        = NS (ARCH,__VA_ARGS__) pair<A,B>;      \
    template<class T, std::size_t N>  using array       = NS (ARCH,__VA_ARGS__) array<T,N>;     \
    template<class T>                 using allocator   = NS (ARCH,__VA_ARGS__) allocator<T>;   \
    template<class T,typename Alloc=allocator<T>>  using vector      = NS (ARCH,__VA_ARGS__) vector<T,Alloc>;\
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
    template< std::size_t I, class... Types > static auto get(const tuple<Types...>& t ) noexcept  { return NSC(ARCH,__VA_ARGS__) get<I>(t); } \
    template< std::size_t I, class... Types > static auto get(      tuple<Types...>& t ) noexcept  { return NSC(ARCH,__VA_ARGS__) get<I>(t); } \
    template<typename ...Ts>                  static auto make_tuple (Ts... t)      { return NSC(ARCH,__VA_ARGS__) make_tuple(t...); }\
    template<typename A, typename B>          static auto make_pair  (A&& a, B&& b) { return NSC(ARCH,__VA_ARGS__) make_pair(a,b); }\
    template<typename T>                      static auto make_reverse_iterator (T&& t) { return NSC(ARCH,__VA_ARGS__) make_reverse_iterator(t); }\
    template <class T>                        static auto swap (T& a, T& b)  { NSC(ARCH,__VA_ARGS__)  swap(a,b);  }\
    template <class T>                        static auto max (T& a, T& b)  { return NSC(ARCH,__VA_ARGS__) max(a,b);  }\
    template <class T>                        static auto min (T& a, T& b)  { return NSC(ARCH,__VA_ARGS__) min(a,b);  }\

// A small utility for defining type traits shortcuts related to one arch.
template<class ARCH, template<typename> typename C, typename T>
using ArchTypeTrait = typename C<ARCH>::template type<T>;

////////////////////////////////////////////////////////////////////////////////
// Default allocator type => uses the ARCH::allocator type
template<class ARCH>
struct DefaultAllocator {
    template<typename T>
    using type = typename ARCH::template allocator<T>;
};

// We define a 'fixed' allocator => for UPMEM, allows to have a fixed number of items in vector cache.
// Through inheritance, we set it as the default one defined for ARCH
// (which will be std::allocator in case of multicore arch)
template<typename ARCH,int NBITEMS_LOG2>
struct FixedAllocator : DefaultAllocator<ARCH> {};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
