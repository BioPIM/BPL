////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct InplaceVector1
{
    USING(ARCH);

    using type = inplace_vector<uint32_t,100>;

#ifdef DPU
    static_assert (bpl::is_serializable_v<type>);
#endif

    auto operator() (type v)  {
    	uint64_t res=0;
    	for (size_t i=0; i<v.size(); i++)  {  res += v[i];  }
    	return res;
    }

    static uint64_t reduce (uint64_t a, uint64_t b)  { return a+b; }
};
