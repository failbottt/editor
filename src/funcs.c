#include "funcs.h"
#include "editor.h"

u64
insert_at_end_of_line(buffer *b, u64 line)
{
    u64 offset = buffer_line_start(b, line) +
        buffer_line_len(b, line);

    E.mode = EDITOR_INSERT_MODE;

    return(offset);
}

void
join_lines(buffer *b, u64 line)
{
    u64 join_off;
    string space = {.s = (u8*)" ", .len = 1};

    if (line + 1 >= b->lines.count)
    {
        return;
    }

    join_off = buffer_line_start(b, line) + buffer_line_len(b, line);
    buffer_delete(b, join_off, buffer_line_start(b, line + 1) - join_off);
    buffer_insert(b, join_off, space);
}

u64
insert_above_current_line(buffer *b, u64 line)
{
    u64 insert_off;

    if (line == 0)
    {
        insert_off = buffer_line_start(b, 0);
    }
    else
    {
        u64 l = line - 1;
        insert_off =
            buffer_line_start(b, l) +
            (buffer_line_len(b, l) + 1);
    }

    string nl = {.s = (u8*)"\n", .len = 1};
    buffer_insert(b, insert_off, nl);

    return(insert_off);
}

u64
insert_below_current_line(buffer *b, u64 line)
{
    u64 insert_off =
        buffer_line_start(b, line) +
        buffer_line_len(b, line);
    string nl = {.s = (u8*)"\n", .len = 1};
    buffer_insert(b, insert_off, nl);

    return(insert_off);
}
