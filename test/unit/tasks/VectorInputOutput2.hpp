////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorInputOutput2
{
    USING(ARCH);

    using vector_t = vector<uint32_t>;

    // IMPORTANT !!!
    // When using ArchUpmem, it seems that the compiler behaves differently according
    // to the presence of the return type in the prototype:
    //   (1) if 'auto' is used only, a (move) copy of the result is done
    //   (2) if explicit type 'vector_t' is used, no copy is done.
    // There is no explanation for this behavior for the moment.
    // Note that there is no such behavior when using ArchMulticore.
    auto operator() (const vector_t& v)  -> vector_t
    {
        vector_t result;

        for (uint32_t i=0; i<v.size(); i++)
        {
            result.push_back (2*v[i]);
        }

        return result;
    }

    using result_type = bpl::core::return_t<decltype(&VectorInputOutput2<ARCH>::operator())>;
    static_assert (std::is_same_v<result_type,vector_t>);
};
