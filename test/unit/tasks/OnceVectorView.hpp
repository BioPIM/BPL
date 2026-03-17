////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

struct SomeData
{
	size_t n=0;
#ifndef DPU
	SomeData(uint32_t n) : n(n)  {}
	SomeData() 				   = delete;
	SomeData(SomeData const&)  = delete;
	SomeData(SomeData &&    )  = default;
#endif
};

template<class ARCH>
struct OnceVectorView
{
    USING(ARCH);

	uint64_t dosomething (auto const& ref) const {
		uint64_t res=0;
		// using iterators (instead of random access) leads to a clang++ crash...
		for (size_t i=0; i<ref.size(); i++)  { res += ref[i].n; }
		return res;
	}

	uint64_t operator() (once<vector_view<SomeData> const&> ref)  const {
    	return dosomething(*ref);
    }
};
