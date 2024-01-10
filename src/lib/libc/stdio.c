#include "stdio.h"

#define UNUSED(var) ((void) var)

/* === File and vtable setup for string-only operation (sprintf, etc.) === */

static int fflush_noop(FILE *stream)
{
    UNUSED(stream);
    return 0;
}

static const struct _file_vtable FILE_VTABLE_STRING_IO = {
        .fflush = fflush_noop,
};

static void file_init_string_io(FILE *stream, char *buf, size_t bufsz)
{
    *stream = (struct _file_impl){
            .vtable = &FILE_VTABLE_STRING_IO,
    };
    setvbuf(stream, buf, _IOFBF, bufsz);
}

