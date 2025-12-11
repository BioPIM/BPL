////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2025
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#pragma once

////////////////////////////////////////////////////////////////////////////////
template<class DERIVED>
struct TemplateSpecializationCRTP
{
    USING (bpl::FirstTemplateParameter_t<DERIVED>);

    struct foo { vector<int> v; };

    auto operator() () const  {
        foo result;
        DERIVED::init (result);
        return result;
    }
};

////////////////////////////////////////////////////////////////////////////////
template<class ARCH>
struct TemplateSpecialization : TemplateSpecializationCRTP<TemplateSpecialization<ARCH>>
{
    static void init (auto& f)  {  /* default implementation */ }
};

#ifdef DPU
// Template specialization for ArchUpmemResources (itself a template class through CFG type)
template<typename CFG>
struct TemplateSpecialization<bpl::ArchUpmemResources<CFG>>
  : TemplateSpecializationCRTP<TemplateSpecialization<bpl::ArchUpmemResources<CFG>>>
{
    static void init (auto& f)  {  f.v.push_back (1);  }
};
#endif
