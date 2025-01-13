////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <tasks/VectorMany2.hpp>

//////////////////////////////////////////////////////////////////////
template<class ARCH>
struct VectorMany2b : VectorMany2<ARCH>
{
    USING (ARCH);

    using type = typename VectorMany2<ARCH>::type;

    auto operator() (const type& t)
    {
        vector<uint32_t> c;

        auto fill = [&] (auto&& obj)
        {
            bpl::class_fields_iterate (obj, [&] (auto&& field)
            {
                for (auto x : field)   { c.push_back(x); }
            });
        };

        fill (t);

        return c;
    }
};
