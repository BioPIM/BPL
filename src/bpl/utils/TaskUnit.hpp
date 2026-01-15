////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2024
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

#include <cstddef>
#include <memory>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** \brief A TaskUnit object represent a set of units that can execute some task. It could be
 * in real life a thread (for multicore arch) or a thread (for upmem arch).
 *
 * A TaskUnit is made of several components, each one representing a set of execution units.
 * For upmem, a component can be a rank or a DPU.
 *
 * This class is a template one where T is the type of the component (rank or dpu for instance);
 * such a component should know how many exec units it provides; for instance, a dpu is supposed
 * to have 16 tasklets for instance.
 */
class TaskUnit
{
public:

    /** Constructor
     * \param nbComponents: the number of components
     */
    TaskUnit (std::size_t nbComponents) : nbComponents_(nbComponents) {}

    /** Destructor */
    virtual ~TaskUnit() {}

    virtual const char* name() const = 0;

    /** Return the number of components
     * \return the number of components
     */
    std::size_t getNbComponents() const { return nbComponents_; }

    virtual std::size_t getNbUnits() const = 0;

private:
    std::size_t nbComponents_ = 0;
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
