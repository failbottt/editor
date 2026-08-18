#include "editor.h"
#include "keys.h"
#include "buffer.h"
#include "cmd.h"
#include "base.h"

static u64
editor_cursor_offset(view *v, buffer *b)
{
    return buffer_line_col_to_offset(b, v->cursor.y, v->cursor.x);
}
static void
editor_set_cursor_from_offset(view *v, buffer *b, u64 offset)
{
    buffer_offset_to_line_col(b, offset, &v->cursor.y, &v->cursor.x);
}

static void
editor_clamp_cursor_x(view *v, buffer *b)
{
    u64 line_len = buffer_line_len(b, v->cursor.y);
    u64 max_x = 0;

    if (line_len > 0)
    {
        max_x = line_len - 1;
    }

    if (v->cursor.x > max_x)
    {
        v->cursor.x = max_x;
    }
}

static void
editor_clear_pending_op(void)
{
    E.mode = EDITOR_NORMAL_MODE;
    E.pending_op = 0;
    E.pending_op_stage = 0;
}

u64
editor_read_key(int fd)
{
    int nread;
    char c, seq[3];
    while ((nread = read(fd,&c,1)) == 0);
    if (nread == -1) exit(1);

    while(1) {
        switch(c) {
        case ESC:    /* escape sequence */
            /* If this is just an esc, we'll timeout herE. */
            if (read(fd,seq,1) == 0) return ESC;
            if (read(fd,seq+1,1) == 0) return ESC;

            /* esc [ sequences. */
            if (seq[0] == '[') {
                if (seq[1] >= '0' && seq[1] <= '9') {
                    /* Extended escape, read additional bytE. */
                    if (read(fd,seq+2,1) == 0) return ESC;
                    if (seq[2] == '~') {
                        switch(seq[1]) {
                        case '3': return DEL_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        }
                    }
                } else {
                    switch(seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                    }
                }
            }

            /* esc O sequences. */
            else if (seq[0] == 'O') {
                switch(seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY; }
            }
            break;
        default:
            return c;
        }
    }
}

static void view_scroll_to_cursor(view *v)
{
    if ((int)v->cursor.y < v->rowoff)
    {
        v->rowoff = (int)v->cursor.y;
    }

    if ((int)v->cursor.y >= v->rowoff + E.screenrows)
    {
        v->rowoff = (int)v->cursor.y - E.screenrows + 1;
    }
}

static u64
editor_line_first_nonblank_offset(buffer *b, u64 line)
{
    u64 start = buffer_line_start(b, line);
    u64 end = start + buffer_line_len(b, line);
    u64 offset = start;

    while (offset < end)
    {
        u8 c = buffer_byte_at(b, offset);

        if (c != ' ' && c != '\t')
        {
            break;
        }

        offset++;
    }

    return offset;
}

static int
editor_is_blank_char(u8 c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int
editor_is_keyword_char(u8 c)
{
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '_';
}

typedef enum {
    EDITOR_WORD_BLANK = 0,
    EDITOR_WORD_KEYWORD,
    EDITOR_WORD_OTHER,
} editor_word_class;

static editor_word_class
editor_classify_char(u8 c)
{
    if (editor_is_blank_char(c))
    {
        return EDITOR_WORD_BLANK;
    }

    if (editor_is_keyword_char(c))
    {
        return EDITOR_WORD_KEYWORD;
    }

    return EDITOR_WORD_OTHER;
}

static u64
editor_skip_word_forward(buffer *b, u64 offset)
{
    u64 limit = b->total_len;
    editor_word_class class;

    if (offset >= limit)
    {
        return limit;
    }

    if (editor_classify_char(buffer_byte_at(b, offset)) != EDITOR_WORD_BLANK)
    {
        class = editor_classify_char(buffer_byte_at(b, offset));
        while (offset < limit && editor_classify_char(buffer_byte_at(b, offset)) == class)
        {
            offset++;
        }
    }

    while (offset < limit && editor_is_blank_char(buffer_byte_at(b, offset)))
    {
        offset++;
    }

    return offset;
}

static u64
editor_skip_word_backward(buffer *b, u64 offset)
{
    editor_word_class class;

    if (b->total_len == 0 || offset == 0)
    {
        return 0;
    }

    if (offset >= b->total_len)
    {
        offset = b->total_len - 1;
    }

    if (editor_classify_char(buffer_byte_at(b, offset)) == EDITOR_WORD_BLANK)
    {
        while (offset > 0 && editor_is_blank_char(buffer_byte_at(b, offset)))
        {
            offset--;
        }

        if (editor_classify_char(buffer_byte_at(b, offset)) == EDITOR_WORD_BLANK)
        {
            return 0;
        }
    }

    class = editor_classify_char(buffer_byte_at(b, offset));

    if (offset > 0 && editor_classify_char(buffer_byte_at(b, offset - 1)) == class)
    {
        while (offset > 0 && editor_classify_char(buffer_byte_at(b, offset - 1)) == class)
        {
            offset--;
        }
        return offset;
    }

    if (offset == 0)
    {
        return 0;
    }

    offset--;

    while (offset > 0 && editor_is_blank_char(buffer_byte_at(b, offset)))
    {
        offset--;
    }

    if (editor_classify_char(buffer_byte_at(b, offset)) == EDITOR_WORD_BLANK)
    {
        return 0;
    }

    class = editor_classify_char(buffer_byte_at(b, offset));

    while (offset > 0 && editor_classify_char(buffer_byte_at(b, offset - 1)) == class)
    {
        offset--;
    }

    return offset;
}

static u64
editor_skip_word_end(buffer *b, u64 offset)
{
    u64 limit = b->total_len;
    editor_word_class class;

    if (limit == 0)
    {
        return 0;
    }

    if (offset >= limit)
    {
        offset = limit - 1;
    }

    if (editor_classify_char(buffer_byte_at(b, offset)) == EDITOR_WORD_BLANK)
    {
        while (offset < limit && editor_is_blank_char(buffer_byte_at(b, offset)))
        {
            offset++;
        }

        if (offset >= limit)
        {
            return limit;
        }
    }

    class = editor_classify_char(buffer_byte_at(b, offset));

    while (offset + 1 < limit &&
           editor_classify_char(buffer_byte_at(b, offset + 1)) == class)
    {
        offset++;
    }

    if (offset + 1 < limit)
    {
        u64 next = offset + 1;

        while (next < limit && editor_is_blank_char(buffer_byte_at(b, next)))
        {
            next++;
        }

        if (next < limit)
        {
            while (next + 1 < limit &&
                   editor_classify_char(buffer_byte_at(b, next + 1)) ==
                   editor_classify_char(buffer_byte_at(b, next)))
            {
                next++;
            }

            return next;
        }
    }

    return offset;
}

typedef struct {
    u64 start;
    u64 end;
} editor_range;

static editor_range
editor_inner_word_range(buffer *b, u64 offset)
{
    editor_range range = {0};
    editor_word_class class;

    if (b->total_len == 0)
    {
        return range;
    }

    if (offset >= b->total_len)
    {
        offset = b->total_len - 1;
    }

    if (editor_classify_char(buffer_byte_at(b, offset)) == EDITOR_WORD_BLANK)
    {
        while (offset < b->total_len &&
               editor_classify_char(buffer_byte_at(b, offset)) == EDITOR_WORD_BLANK)
        {
            offset++;
        }

        if (offset >= b->total_len)
        {
            return range;
        }
    }

    class = editor_classify_char(buffer_byte_at(b, offset));
    range.start = offset;
    range.end = offset + 1;

    while (range.start > 0 &&
           editor_classify_char(buffer_byte_at(b, range.start - 1)) == class)
    {
        range.start--;
    }

    while (range.end < b->total_len &&
           editor_classify_char(buffer_byte_at(b, range.end)) == class)
    {
        range.end++;
    }

    return range;
}

void
editor_move_cursor(u64 key)
{
    view *v = &E.views[0];
    buffer *b = &E.buffers[v->buffer_id];

    switch (key) {
        case KEY_J:
            if (v->cursor.y + 1 < b->lines.count)
            {
                v->cursor.y++;
                editor_clamp_cursor_x(v, b);
            }
            view_scroll_to_cursor(v);
            break;
        case KEY_K:
            if (v->cursor.y > 0)
            {
                v->cursor.y--;
                editor_clamp_cursor_x(v, b);
            }
            view_scroll_to_cursor(v);
            break;
        case KEY_L:
            {
                u64 line_len = buffer_line_len(b, v->cursor.y);
                if (v->cursor.x < line_len)
                {
                    v->cursor.x++;
                }
                break;
            }
        case KEY_H:
            if (v->cursor.x > 0) v->cursor.x--;
            break;
    }
}

void
editor_process_keypress(int c) {

    /* @cleanup: hardcoded */
    view *v = &E.views[0];
    buffer *buf = &E.buffers[v->buffer_id];

    if (E.mode == EDITOR_NORMAL_MODE)
    {
        switch(c) {
        case 'j':
        case 'k':
        case 'h':
        case 'l':
            {
                editor_move_cursor(c);
                break;
            }
        case 'u':
            {
                /* undo */
                break;
            }
        case 'V':
            {
                /*visual line select*/
                break;
            }
        case ':':
            {
                E.mode = EDITOR_COMMAND_MODE;
                u8 *colon = (u8*)":";
                arena_push_array(&E.cmd, colon, 1);
                break;
            }
        case 'b':
            {
                u64 offset = editor_cursor_offset(v, buf);
                offset = editor_skip_word_backward(buf, offset);
                editor_set_cursor_from_offset(v, buf, offset);
                view_scroll_to_cursor(v);
                break;
            }
        case 'w':
            {
                u64 offset = editor_cursor_offset(v, buf);
                offset = editor_skip_word_forward(buf, offset);
                editor_set_cursor_from_offset(v, buf, offset);
                view_scroll_to_cursor(v);
                break;
            }
        case 'e':
            {
                u64 offset = editor_cursor_offset(v, buf);

                offset = editor_skip_word_end(buf, offset);
                editor_set_cursor_from_offset(v, buf, offset);
                view_scroll_to_cursor(v);
                break;
            }
        case 'i':
            {
                E.mode = EDITOR_INSERT_MODE;
                break;
            }
        case 'I':
            {
                u64 insert_off = editor_line_first_nonblank_offset(buf, v->cursor.y);

                E.mode = EDITOR_INSERT_MODE;
                editor_set_cursor_from_offset(v, buf, insert_off);
                view_scroll_to_cursor(v);
                break;
            }
        case 'A':
            {
                u64 line = v->cursor.y;
                u64 insert_off = buffer_line_start(buf, line) + buffer_line_len(buf, line);
                E.mode = EDITOR_INSERT_MODE;
                editor_set_cursor_from_offset(v, buf, insert_off);
                break;
            }
        case 'a':
            {
                u64 line_len = buffer_line_len(buf, v->cursor.y);
                editor_set_cursor_from_offset(v, buf, v->cursor.x + 1);
                E.mode = EDITOR_INSERT_MODE;
                break;
            }
        case 'O':
            {
                u64 line = v->cursor.y - 1;
                u64 insert_off =
                    buffer_line_start(buf, line) + buffer_line_len(buf, line);
                string nl = {.s = (u8*)"\n", .len = 1};
                buffer_insert(buf, insert_off, nl);

                E.mode = EDITOR_INSERT_MODE;
                editor_set_cursor_from_offset(v, buf, insert_off + 1);
                view_scroll_to_cursor(v);
                break;
            }
        case 'o':
            {
                u64 line = v->cursor.y;
                u64 insert_off =
                    buffer_line_start(buf, line) + buffer_line_len(buf, line);
                string nl = {.s = (u8*)"\n", .len = 1};
                buffer_insert(buf, insert_off, nl);

                E.mode = EDITOR_INSERT_MODE;
                editor_set_cursor_from_offset(v, buf, insert_off + 1);
                view_scroll_to_cursor(v);
                break;
            }
        case 'g':
        case 'd':
        case 'c':
        case 'r':
        case 'y':
            {
                E.mode = EDITOR_PENDING_OP_MODE;
                E.pending_op = c;
                break;
            }
        case 'G':
            {
                editor_set_cursor_from_offset(v, buf, buf->total_len - 1);
                view_scroll_to_cursor(v);
                break;
            }
        case CTRL_F:
            {
                u64 max_rowoff = 0;

                if (buf->lines.count > (u64)E.screenrows)
                {
                    max_rowoff = buf->lines.count - E.screenrows;
                }

                if ((u64)v->rowoff + E.screenrows < buf->lines.count)
                {
                    v->rowoff += E.screenrows;
                    if ((u64)v->rowoff > max_rowoff)
                    {
                        v->rowoff = (int)max_rowoff;
                    }
                }
                else
                {
                    v->rowoff = (int)max_rowoff;
                }

                v->cursor.y = (u64)v->rowoff;
                editor_clamp_cursor_x(v, buf);
                break;

            }
        case CTRL_B:
            {
                if (v->rowoff > 0)
                {
                    if (v->rowoff >= E.screenrows)
                    {
                        v->rowoff -= E.screenrows;
                    }
                    else
                    {
                        v->rowoff = 0;
                    }
                }

                v->cursor.y = (u64)(v->rowoff + E.screenrows - 1);
                if (v->cursor.y >= buf->lines.count)
                {
                    v->cursor.y = buf->lines.count - 1;
                }

                editor_clamp_cursor_x(v, buf);
                break;
            }
        case ESC:
            break;
        default:
            break;
        }

        return;
    }

    if (E.mode == EDITOR_INSERT_MODE)
    {
        u64 offset = editor_cursor_offset(v, buf);

        switch (c) {
        case ESC:
            E.mode = EDITOR_NORMAL_MODE;
            break;
        case BACKSPACE:
        case DEL_KEY:
            if (offset > 0)
            {
                buffer_delete(buf, offset - 1, 1);
                editor_set_cursor_from_offset(v, buf, offset - 1);
                view_scroll_to_cursor(v);
            }
            break;
        case TAB:
            {
                /* 4 spaces */
                u8* tab = (u8*)"    ";
                string text = {.s = tab, .len = 4};
                buffer_insert(buf, offset, text);
                editor_set_cursor_from_offset(v, buf, offset + text.len);
                view_scroll_to_cursor(v);
                break;
            }
        case ENTER:
            {
                break;
            }
        default:
            if (c >= 32 && c <= 126)
            {
                u8 ch = (u8)c;
                string text = {.s = &ch, .len = 1};
                buffer_insert(buf, offset, text);
                editor_set_cursor_from_offset(v, buf, offset + 1);
                view_scroll_to_cursor(v);
            }
            break;
        }
        return;
    }

    if (E.mode == EDITOR_PENDING_OP_MODE)
    {
        if (c == ESC)
        {
            editor_clear_pending_op();
            return;
        }

        if (E.pending_op == 'y')
        {

            if (c == 'i')
            {
                E.pending_op_stage = 1;
                return;
            }
            editor_clear_pending_op();
            return;
        }

        if (E.pending_op == 'd')
        {
            if (E.pending_op_stage == 0)
            {
                if (c == 'i')
                {
                    E.pending_op_stage = 1;
                    return;
                }

                editor_clear_pending_op();
                return;
            }

            if (E.pending_op_stage == 1)
            {
                if (c == 'w')
                {
                    editor_range range = editor_inner_word_range(buf, editor_cursor_offset(v, buf));

                    if (range.end > range.start)
                    {
                        buffer_delete(buf, range.start, range.end - range.start);
                        editor_set_cursor_from_offset(v, buf, range.start);
                        editor_clamp_cursor_x(v, buf);
                        view_scroll_to_cursor(v);
                    }

                    editor_clear_pending_op();
                    return;
                }

                editor_clear_pending_op();
                return;
            }
        }

        editor_clear_pending_op();
        return;
    }

    if (E.mode == EDITOR_VISUAL_MODE)
    {
        return;
    }

    if (E.mode == EDITOR_COMMAND_MODE)
    {
        switch (c) {
        case ESC:
            E.mode = EDITOR_NORMAL_MODE;
            E.cmd.cur_pos = 0;
            break;
        case BACKSPACE:
            if (E.cmd.cur_pos > 0)
            {
                E.cmd.cur_pos--;
            }
            break;
        case ENTER:
            {
                cmd_process(&E.cmd);
                break;
            }
        default:
            if (c >= 32 && c <= 126)
            {
                u8 ch = (u8)c;
                arena_push_array(&E.cmd, &ch, 1);
            }
            break;
        }
    }
}

void editor_set_cmd_status_message(u8 *msg)
{
    size_t len;

    if (msg == NULL)
    {
        E.status_message[0] = '\0';
        return;
    }

    len = strlen((char *)msg);
    if (len >= sizeof(E.status_message))
    {
        len = sizeof(E.status_message) - 1;
    }

    memcpy(E.status_message, msg, len);
    E.status_message[len] = '\0';
}

buffer* editor_active_buffer()
{
    view *v = &E.views[0];
    return &E.buffers[v->buffer_id];
}
