////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/arch/Arch.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** \brief Dummy architecture that doesn't refer any real architecture.
 *
 * This class is mainly used from the host side when one needs to retrieve
 * type information from a task without real dependency on the arch. For
 * instance, one could write from host side:
 * @code
 * typename TASK<ArchDummy>::type t;
 * @endcode
 */
struct ArchDummy
{
    using config_t = void;

    // Factory that returns a type with a specific configuration.
    template<typename CFG=void> using factory = ArchDummy;

    template<typename T, typename S=T> using pair = std::pair<T,S>;

    template<typename T, int N> using array  = std::array<T,N>;

    template<typename T, typename...Ts> using vector = std::vector<T, Ts...>;

    template<typename T>        using span   = std::span<T>;

    template<typename T>        using vector_view = std::vector<T>;

    template<typename T>        using allocator = std:: allocator<T>;

    template<typename T>        using initializer_list = std::initializer_list<T>;

    template<typename ...Ts>    using tuple  = std::tuple<Ts...>;

    template<typename T, typename S=T>
    static auto make_pair (T t, S s) { return std::make_pair(t,s); }

    template<typename T>
    static auto make_reverse_iterator (T&& t) { return std::make_reverse_iterator(t); }

    template<typename ...Ts>
    static auto make_tuple (Ts... t) { return std::make_tuple(t...); }

    template< std::size_t I, class... Types >
    static auto get(const tuple<Types...>& t ) noexcept  { return std::get<I>(t); }

    using string = std::string;
    using size_t = std::size_t;

    template <typename ...Args>
    static void info (const char * format, Args ...args)  {  printf(format, args...);  }

    static auto round (int n)  {  return (n + 8 - 1) & -8;; }

    using mutex = std::mutex;
    template<typename T> using lock_guard = std::lock_guard<T>;

    template <class T> static void swap ( T& a, T& b ) {  std::swap(a,b);  }
    template <class T> static T max ( T& a, T& b ) {  return std::max(a,b);  }
    template <class T> static T min ( T& a, T& b ) {  return std::min(a,b);  }

    template<typename T> using once   = bpl::once<T>;
    template<typename T> using global = bpl::global<T>;
};

////////////////////////////////////////////////////////////////////////////////
};
////////////////////////////////////////////////////////////////////////////////
