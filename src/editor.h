#ifndef EDITOR_H
#define EDITOR_H

#include "base.h"
#include "buffer.h"

typedef struct
{
    u64 running;
    buffer *buffers;

    /* @cleanup: tmp */
    u64 current_buffer;

} editor;

extern editor E;

void editor_init(string file_path);

#endif
