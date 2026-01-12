////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <vector>
#include <bpl/utils/metaprog.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** \brief Check whether a class has a 'reduce' method.
 */
template <typename T>
class has_reduce
{
    typedef char one;
    struct two { char x[2]; };
    template <typename C> static one test( decltype(C::reduce) ) ;
    template <typename C> static two test(...);

public:
    enum { value = sizeof(test<T>(0)) == sizeof(char) };
};

////////////////////////////////////////////////////////////////////////////////

/** \brief struct that overloads operator() by potentially transforming
 * the incoming result through a reduce process.
 *
 * Generic definition
 */
template<bool,class TASK>  struct Reduce {};

/** Specialization 1: reduce the partial results by using 'reduce' method */
template<class TASK>  struct Reduce<true,TASK>
{
    using Result_t = return_t <decltype(&TASK::operator())>;

    template<typename RESULT_RANGE>
    auto operator() (const RESULT_RANGE& results) const
    {
        Result_t res = Result_t();
        for (auto&& result: results)  {  res = TASK::reduce(res,result);  }
        return res;
    }
};

/** Specialization 2: return the partial results */
template<class TASK>  struct Reduce<false,TASK>
{
    using Result_t = return_t <decltype(&TASK::operator())>;

    template<typename RESULT_RANGE>
    auto& operator() (RESULT_RANGE& results)
    {
        return results;
    }
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
