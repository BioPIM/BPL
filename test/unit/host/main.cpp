////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <cstddef>
#include <cstdint>
#include <bpl/utils/metaprog.hpp>
#include <bpl/core/Task.hpp>
#include <bpl/arch/ArchMulticore.hpp>

////////////////////////////////////////////////////////////////////////////////
// we need a main.cpp file not for define the 'main' function but rather to
// initialize static data
////////////////////////////////////////////////////////////////////////////////

using resources_t = bpl::arch::ArchMulticore;
using TaskBase_t  = bpl::core::TaskBase<bpl::core::Task<resources_t,1>>;

template<>  decltype(TaskBase_t::mutexes)  TaskBase_t::mutexes = { std::mutex{} };
