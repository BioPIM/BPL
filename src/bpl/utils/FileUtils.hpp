////////////////////////////////////////////////////////////////////////////////
// BPL, the Process In Memory library for bioinformatics 
// date  : 2023
// author: edrezen
////////////////////////////////////////////////////////////////////////////////

#include <firstinclude.hpp>

#ifndef __BPL_FILE_UTILS__
#define __BPL_FILE_UTILS__

#include <cstdio>

////////////////////////////////////////////////////////////////////////////////
namespace bpl  {
namespace core {
////////////////////////////////////////////////////////////////////////////////

class BlackHoleFile
{
public:

    static FILE* get()
    {
        cookie_io_functions_t  memfile_func = {
            .read  = memfile_read,
            .write = memfile_write,
            .seek  = memfile_seek,
            .close = memfile_close
        };

        return fopencookie (nullptr, "w+", memfile_func);
    }

private:

    static ssize_t  memfile_write (void *c, const char *buf, size_t size)  { return 0;  }
    static ssize_t  memfile_read  (void *c, char *buf, size_t size)        { return 0;  }
    static int      memfile_seek  (void *c, off64_t *offset, int whence)   { return 0;  }
    static int      memfile_close (void *c)                                { return 0;  }
};

////////////////////////////////////////////////////////////////////////////////
} };  // end of namespace
////////////////////////////////////////////////////////////////////////////////

#endif // __BPL_FILE_UTILS__
