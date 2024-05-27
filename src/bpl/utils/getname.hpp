////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_GET_NAME__
#define __BPL_GET_NAME__

////////////////////////////////////////////////////////////////////////////////
// Get the name  of a class (at compilation time)
// see https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/
////////////////////////////////////////////////////////////////////////////////

#include <string_view>
#include <array>   // std::array
#include <utility> // std::index_sequence

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

template <std::size_t...Idxs>
constexpr auto substring_as_array(std::string_view str, std::index_sequence<Idxs...>)
{
    return std::array{str[Idxs]..., '\n'};
}

template <typename T>
constexpr auto type_name_array()
{
#if defined(__clang__)
    constexpr auto prefix   = std::string_view{"[T = "};
    constexpr auto suffix   = std::string_view{"]"};
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(__GNUC__)
    constexpr auto prefix   = std::string_view{"with T = "};
    constexpr auto suffix   = std::string_view{"]"};
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
    constexpr auto prefix   = std::string_view{"type_name_array<"};
    constexpr auto suffix   = std::string_view{">(void)"};
    constexpr auto function = std::string_view{__FUNCSIG__};
#else
# error Unsupported compiler
#endif

    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end = function.rfind(suffix);

    static_assert(start < end);

    constexpr auto name = function.substr(start, (end - start));
    return substring_as_array(name, std::make_index_sequence<name.size()>{});
}

template <typename T>
struct type_name_holder {
    static inline constexpr auto value = type_name_array<T>();
};

template <typename T>
constexpr auto type_name() -> std::string_view
{
    constexpr auto& value = type_name_holder<T>::value;

    auto str = std::string_view{value.data(), value.size()};

    auto trimL = str.find_first_not_of("\t\n\r ");
    auto trimR = str.find_last_not_of ("\t\n\r ") + 1;

    if (trimL == std::string_view::npos)  { trimL = 0;          }
    if (trimR == std::string_view::npos)  { trimR = str.size(); }

    return str.substr (trimL,trimR);
}

template <typename T>
constexpr auto type_shortname() -> std::string_view
{
    constexpr auto& value = type_name_holder<T>::value;

    auto str = std::string_view{value.data(), value.size()};

    auto pos = str.find('<');
    return  (pos != std::string_view::npos) ? str.substr(0,pos) : str;
}

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif // __BPL_GET_NAME__