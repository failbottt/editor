#include "cmd.h"
#include "editor.h"
#include "buffer.h"

void cmd_move_cursor_right()
{
    u64 cursor_offset = E.cursor_offset;
    buffer *b = E.active_buffer;

    u64 doc_length = buffer_document_length(b);

    if (cursor_offset == 0 && doc_length == 0)
    {
        return;
    }

    if (cursor_offset == 0 && doc_length > 0)
    {
        E.cursor_offset++;
        return;
    }

    u64 i;

    for (i = 0; i < b->cached_line_starts.len; i++)
    {
        u64 start = b->cached_line_starts.indexes[i];

        /*
         *
         */
        u8 next_char_is_end_of_line = ((cursor_offset+1) == start);
        if (next_char_is_end_of_line)
        {
            return;
        }
    }

    E.cursor_offset++;
}

void cmd_move_cursor_left()
{
    u64 cursor_offset = E.cursor_offset;
    buffer *b = E.active_buffer;

    if (cursor_offset == 0)
    {
        return;
    }

    int i;

    for(i = b->cached_line_starts.len - 1; i > 0; i--)
    {
        u64 start = b->cached_line_starts.indexes[i];

        u8 prev_char_is_end_of_line = ((cursor_offset-1) == start);
        if (prev_char_is_end_of_line)
        {
            return;
        }
    }

    E.cursor_offset--;
}
