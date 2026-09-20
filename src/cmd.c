#include "cmd.h"
#include "editor.h"
#include "buffer.h"

void cmd_move_cursor_to_first_char_on_line()
{
    u64 cursor_offset = E.cursor_offset;
    buffer *b = E.active_buffer;

    cursor_pos cursor = buffer_offset_to_screen_pos(b, cursor_offset);

    u64 y = cursor.y - 1;
    u64 x = cursor.x - 1;

    if (x < 0) x = 0;
    if (y < 0) y = 0;

    u64 current_line_start = b->cached_line_starts.offsets[y];
    u64 next_line_start = b->cached_line_starts.offsets[y + 1];

    u64 i = current_line_start;
    while (i < next_line_start)
    {
        u8 c = buffer_char_at_offset(b, i);
        if (c != '\n' && c != '\t' && c != ' ')
        {
            break;
        }

        i++;
    }

    E.cursor_offset = i;
}

void cmd_move_cursor_to_end_of_line()
{
    u64 cursor_offset = E.cursor_offset;
    buffer *b = E.active_buffer;

    u64 doc_len = buffer_document_length(b);

    cursor_pos cursor = buffer_offset_to_screen_pos(b, cursor_offset);

    u64 y = cursor.y - 1;
    u64 x = cursor.x - 1;

    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (y >= doc_len) y = doc_len - 1;

    u8 on_last_line = (y == b->cached_line_starts.len - 1);
    if (on_last_line)
    {
        E.cursor_offset = doc_len - 1;
        return;
    }

    u64 current_line_start = b->cached_line_starts.offsets[y];
    u64 next_line_start = b->cached_line_starts.offsets[y+1];

    E.cursor_offset = next_line_start - 2;
}

void cmd_delete_character()
{
    u64 cursor_offset = E.cursor_offset;
    buffer *b = E.active_buffer;

    buffer_delete(b, E.cursor_offset-1, E.cursor_offset);
}

void cmd_move_cursor_right()
{
    u64 cursor_offset = E.cursor_offset;
    buffer *b = E.active_buffer;

    u64 doc_length = buffer_document_length(b);

    if (cursor_offset == 0 && doc_length == 0)
    {
        return;
    }

    if (cursor_offset >= doc_length - 1)
    {
        return;
    }

    u64 i;

    for (i = 0; i < b->cached_line_starts.len; i++)
    {
        u64 start = b->cached_line_starts.offsets[i];

        /* @fix: the +2 is likely a mistake */
        u8 next_char_is_end_of_line = ((cursor_offset+2) == start);
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

    cursor_pos cursor = buffer_offset_to_screen_pos(b, E.cursor_offset);

    if (cursor.x == 1)
    {
        return;
    }

    E.cursor_offset--;
}

void cmd_move_cursor_down()
{
    u64 cursor_offset = E.cursor_offset;
    buffer *b = E.active_buffer;
    cursor_pos cursor;
    u64 current_line_index;
    u64 next_line_index;
    u64 current_line_start;
    u64 next_line_start;
    struct line next_line_length;
    u64 desired_column;

    cursor = buffer_offset_to_screen_pos(b, cursor_offset);

    /* @note: the terminal is row,col 1,1 based */
    u8 already_on_last_line = (cursor.y == b->cached_line_starts.len);
    if (already_on_last_line) return;

    current_line_index = cursor.y - 1;
    next_line_index = current_line_index + 1;

    current_line_start = b->cached_line_starts.offsets[current_line_index];
    next_line_start = b->cached_line_starts.offsets[next_line_index];

    next_line_length = buffer_line_length(b, next_line_start);

    desired_column = cursor_offset - current_line_start;
    if (desired_column > next_line_length.len)
    {
        desired_column = next_line_length.len;
    }
    if (desired_column > 0 && desired_column == next_line_length.len)
    {
        desired_column--;
    }

    E.cursor_offset = next_line_start + desired_column;
}

void cmd_move_cursor_up()
{
    u64 cursor_offset = E.cursor_offset;
    buffer *b = E.active_buffer;

    cursor_pos cursor = buffer_offset_to_screen_pos(b, cursor_offset);

    /* @note: the terminal is row,col 1,1 based */
    u8 already_on_first_line = (cursor.y == 1);
    if (already_on_first_line) return;

    u64 current_line_index = cursor.y - 1;
    u64 previous_line_index = current_line_index - 1;

    u64 current_line_start = b->cached_line_starts.offsets[current_line_index];
    u64 previous_line_start = b->cached_line_starts.offsets[previous_line_index];

    u64 desired_column = cursor_offset - current_line_start;

    u64 previous_line_end = current_line_start - 1;
    u64 previous_line_length = previous_line_end - previous_line_start;

    if (desired_column > previous_line_length)
    {
        /* @note: don't want the cursor to end up on the new line chars */
        desired_column = previous_line_length - 1;
    }

    E.cursor_offset = previous_line_start + desired_column;

}
