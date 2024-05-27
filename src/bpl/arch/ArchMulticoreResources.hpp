////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_ARCH_MULTICORE_RESOURCES__
#define __BPL_ARCH_MULTICORE_RESOURCES__
////////////////////////////////////////////////////////////////////////////////

#include <array>
#include <vector>
#include <span>
#include <tuple>
#include <string>
#include <mutex>

#include <bpl/utils/tag.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace arch {
////////////////////////////////////////////////////////////////////////////////

struct ArchMulticoreResources
{
    template<typename T, typename S=T> using pair = std::pair<T,S>;

    template<typename T, int N> using array  = std::array<T,N>;

    template<typename T>        using vector = std::vector<T>;

    template<typename T>        using span   = std::span<T>;

    template<typename T>        using vector_view = std::vector<T>;

    template<typename T>        using initializer_list = std::initializer_list<T>;

    template<typename ...Ts>    using tuple  = std::tuple<Ts...>;

    template<typename T, typename S=T>
    static auto make_pair (T t, S s) { return std::make_pair(t,s); }

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

    template<typename T> using once   = bpl::core::once<T>;
    template<typename T> using global = bpl::core::global<T>;
};

////////////////////////////////////////////////////////////////////////////////
} };
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
#endif // __BPL_ARCH_MULTICORE_RESOURCES__
