////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_REDUCER__
#define __BPL_REDUCER__

#include <bpl/utils/metaprog.hpp>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

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
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif // __BPL_REDUCER_SUM__
