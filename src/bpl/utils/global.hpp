//////////////////////////////////////////////////////////////////////////////////
//// BPL, the Process In Memory library for bioinformatics
//// date  : 2024
//// author: edrezen
//////////////////////////////////////////////////////////////////////////////////
//
//#pragma once
//
//#include <firstinclude.hpp>
//
//////////////////////////////////////////////////////////////////////////////////
///** Structure that encapsulates a function argument or type T.
// */
//template<typename TYPE>
//struct GlobalProxy
//{
//    constexpr GlobalProxy (TYPE&& t) : _t(std::forward<TYPE>(t))  {}
//    operator TYPE& () const { return _t; }
//
//    TYPE _t;
//};
//
//////////////////////////////////////////////////////////////////////////////////
//
//template<int...IDX>
//struct Global
//{
//};
//
//////////////////////////////////////////////////////////////////////////////////
//
////template<typename TYPE>
////constexpr inline auto global (TYPE&& t)
////{
////    return GlobalProxy<TYPE>  {std::forward<TYPE>(t)};
////}
////
////////////////////////////////////////////////////////////////////////////////////
////
////template<typename T>  struct is_global                 : std::false_type {};
////template<typename T>  struct is_global<GlobalProxy<T>> : std::true_type  {};
////
////template<typename T>  inline constexpr bool is_global_v = is_global<T>::value;
////
////////////////////////////////////////////////////////////////////////////////////
//
//namespace bpl
//{
//    template<typename T>
//    struct global  { using type = T; };
//
//    template<typename T>
//    using global_t = typename global<T>::type;
//
//
//    template<typename T>
//    struct split { using type = T; };
//
//    template<typename T>
//    using split_t = typename split<T>::type;
//};
//
