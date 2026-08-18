#include "funcs.h"
#include "editor.h"

u64 insert_append_line(buffer *b, u64 line)
{
    u64 offset = buffer_line_start(b, line) +
        buffer_line_len(b, line);

    E.mode = EDITOR_INSERT_MODE;

    return offset;
}
