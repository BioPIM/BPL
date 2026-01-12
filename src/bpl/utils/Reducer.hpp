////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <bpl/utils/metaprog.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** \brief Function object that can reduce two items into a single one.
 *
 * We could have directly use the functor FCT but we add some static_assert here.
 *
 * \param FCT: the functor that reduces two items.
 */
template<typename FCT>
struct Reducer
{
    using signature_t = FunctionSignatureParser<decltype(&FCT::operator())>;

    static_assert (signature_t::args_size==2);

    using A = typename signature_t::template arg_t<0>;
    using B = typename signature_t::template arg_t<1>;

    // Important: we need to decay the result type -> don't want to return a temporary object (const&)
    static std::decay_t<A> reduce (A a, B b)
    {
        return FCT() (a,b);
    }
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
