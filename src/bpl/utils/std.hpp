////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_UTILS_STD_HPP__
#define __BPL_UTILS_STD_HPP__

// The UPMEM's clang version implies that we use some experimental parts of the std
// (for instance the 'apply' function)
// Since we may want to use a more recent version, the 'experimental' should disappear,
// so, we prefer to encapsulate a little bit here the std functions we want to use in such
// a way that using a newer compiler will imply localized modifications.
#include <experimental/tuple>
#include <experimental/filesystem>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

template< class F, class Tuple>
constexpr decltype(auto) apply( F&& f, Tuple&& t )  {  return std::experimental::apply(f,t); }

using directory_entry              = std::experimental::filesystem::directory_entry;
using recursive_directory_iterator = std::experimental::filesystem::recursive_directory_iterator;

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif /* __BPL_UTILS_STD_HPP__ */