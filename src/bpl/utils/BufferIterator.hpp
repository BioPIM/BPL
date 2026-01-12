////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2026
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#pragma once

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
////////////////////////////////////////////////////////////////////////////////

/** \brief Generic class using a buffer for serialization management.
 *
 * This class is intended to be specialized for specific architectures (
 * see ArchUpmemResources.hpp for instance)
 *
 * \param ARCH: the associated architecture
 */
template<class ARCH>
class BufferIterator
{
public:
    using pointer_t = char*;

    /** Interprets the current location in the buffer as an object of type
     * \return a reference of type T on the object in the buffer
     */
    template<typename T> T& object (std::size_t size) const;

    /** Copy some bytes from the current position in the buffer to a provided
     * target buffer.
     */
    void memcpy (void* target, std::size_t size);

    /** Advance n bytes in the buffer. */
    void advance (std::size_t n);

    /**
     * \return pointer to the current position in the buffer. */
    pointer_t get() ;
};

////////////////////////////////////////////////////////////////////////////////
};  // end of namespace
////////////////////////////////////////////////////////////////////////////////
