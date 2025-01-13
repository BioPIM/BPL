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

/** */
class ArchDummy
{
public:

    template<typename T, typename S=T> using pair = std::pair<T,S>;

    template<typename T, int N> using array  = std::array<T,N>;

    template<typename T>        using vector = std::vector<T>;

    template<typename T>        using initializer_list = std::initializer_list<T>;

    template<typename ...Ts>    using tuple  = std::tuple<Ts...>;

    template<typename T, typename S=T>
    static auto make_pair (T t, S s) { return std::make_pair(t,s); }

    template<typename ...Ts>
    static auto make_tuple (Ts... t) { return std::make_tuple(t...); }

    using string = std::string;
    using size_t = std::size_t;

};

////////////////////////////////////////////////////////////////////////////////
};
////////////////////////////////////////////////////////////////////////////////
