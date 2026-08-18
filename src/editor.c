#include "editor.h"
#include "keys.h"
#include "buffer.h"
#include "cmd.h"
#include "base.h"
#include "term.h"

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

static void
editor_free_register_one(void)
{
    if (E.register_one == NULL)
    {
        return;
    }

    free(E.register_one->s);
    free(E.register_one);
    E.register_one = NULL;
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
editor_process_keypress(int c)
{
    view *v = &E.views[E.active_view];
    buffer *b = &E.buffers[v->buffer_id];

    if (E.mode == EDITOR_NORMAL_MODE)
    {
        switch(c) {
        case 'A':
            {
                u64 line = v->cursor.y;
                u64 insert_off = buffer_line_start(b, line) + buffer_line_len(b, line);
                E.mode = EDITOR_INSERT_MODE;
                editor_set_cursor_from_offset(v, b, insert_off);
                break;
            }
        case 'a':
            {
                u64 line_len = buffer_line_len(b, v->cursor.y);
                editor_set_cursor_from_offset(v, b, v->cursor.x + 1);
                E.mode = EDITOR_INSERT_MODE;
                break;
            }
        case 'b':
            {
                u64 offset = editor_cursor_offset(v, b);
                offset = editor_skip_word_backward(b, offset);
                editor_set_cursor_from_offset(v, b, offset);
                view_scroll_to_cursor(v);
                break;
            }
        case 'J':
            {
                u64 line = v->cursor.y;
                u64 prev_cursor = editor_cursor_offset(v, b);
                u64 join_off;
                string space = {.s = (u8*)" ", .len = 1};

                if (line + 1 >= b->lines.count)
                {
                    break;
                }

                join_off = buffer_line_start(b, line) + buffer_line_len(b, line);
                buffer_delete(b, join_off, buffer_line_start(b, line + 1) - join_off);
                buffer_insert(b, join_off, space);
                editor_set_cursor_from_offset(v, b, prev_cursor);
                view_scroll_to_cursor(v);
                break;
            }
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
        case 'w':
            {
                u64 offset = editor_cursor_offset(v, b);
                offset = editor_skip_word_forward(b, offset);
                editor_set_cursor_from_offset(v, b, offset);
                view_scroll_to_cursor(v);
                break;
            }
        case 'e':
            {
                u64 offset = editor_cursor_offset(v, b);

                offset = editor_skip_word_end(b, offset);
                editor_set_cursor_from_offset(v, b, offset);
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
                u64 insert_off = editor_line_first_nonblank_offset(b, v->cursor.y);

                E.mode = EDITOR_INSERT_MODE;
                editor_set_cursor_from_offset(v, b, insert_off);
                view_scroll_to_cursor(v);
                break;
            }
        case 'O':
            {
                u64 insert_off;

                if (v->cursor.y == 0)
                {
                    insert_off = buffer_line_start(b, 0);
                }
                else
                {
                    u64 line = v->cursor.y - 1;
                    insert_off =
                        buffer_line_start(b, line) + buffer_line_len(b, line) - 1;
                }

                string nl = {.s = (u8*)"\n", .len = 1};
                buffer_insert(b, insert_off, nl);

                E.mode = EDITOR_INSERT_MODE;
                editor_set_cursor_from_offset(v, b, insert_off);
                view_scroll_to_cursor(v);
                break;
            }
        case 'o':
            {
                u64 line = v->cursor.y;
                u64 insert_off =
                    buffer_line_start(b, line) + buffer_line_len(b, line);
                string nl = {.s = (u8*)"\n", .len = 1};
                buffer_insert(b, insert_off, nl);

                E.mode = EDITOR_INSERT_MODE;
                editor_set_cursor_from_offset(v, b, insert_off + 1);
                view_scroll_to_cursor(v);
                break;
            }
        case PASTE:
            {
                u64 line;
                u64 insert_off;

                if (E.paste_newline)
                {
                    line = v->cursor.y;
                    insert_off = buffer_line_start(b, line) +
                        buffer_line_len(b, line);
                    buffer_insert(b, insert_off, *E.register_one);
                    editor_set_cursor_from_offset(v, b, insert_off);
                    view_scroll_to_cursor(v);
                    break;
                }

                line = v->cursor.y;
                insert_off = editor_cursor_offset(v, b);
                buffer_insert(b, insert_off, *E.register_one);
                editor_set_cursor_from_offset(v, b, insert_off);
                view_scroll_to_cursor(v);
                break;
            }
        case PASTE_BEFORE:
            {
                if (E.paste_newline)
                {
                    u64 insert_off = buffer_line_start(b, v->cursor.y);
                    string nl = {.s = (u8*)"\n", .len = 1};
                    buffer_insert(b, insert_off, *E.register_one);
                    buffer_insert(b, insert_off + E.register_one->len, nl);
                    editor_set_cursor_from_offset(v, b, insert_off);
                    view_scroll_to_cursor(v);
                    break;
                }

                u64 insert_off = editor_cursor_offset(v, b);
                buffer_insert(b, insert_off, *E.register_one);
                editor_set_cursor_from_offset(v, b, insert_off);
                view_scroll_to_cursor(v);
                break;
            }
        case 'g':
        case REPLACE:
        case CLEAR:
        case DELETE:
        case YANK:
            {
                E.mode = EDITOR_PENDING_OP_MODE;
                E.pending_op = c;
                break;
            }
        case 'G':
            {
                editor_set_cursor_from_offset(v, b, b->total_len - 1);
                view_scroll_to_cursor(v);
                break;
            }
        case CTRL_F:
            {
                u64 max_rowoff = 0;

                if (b->lines.count > (u64)E.screenrows)
                {
                    max_rowoff = b->lines.count - E.screenrows;
                }

                if ((u64)v->rowoff + E.screenrows < b->lines.count)
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
                editor_clamp_cursor_x(v, b);
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
                if (v->cursor.y >= b->lines.count)
                {
                    v->cursor.y = b->lines.count - 1;
                }

                editor_clamp_cursor_x(v, b);
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
        u64 offset = editor_cursor_offset(v, b);

        switch (c) {
        case ESC:
            E.mode = EDITOR_NORMAL_MODE;
            break;
        case BACKSPACE:
        case DEL_KEY:
            if (offset > 0)
            {
                buffer_delete(b, offset - 1, 1);
                editor_set_cursor_from_offset(v, b, offset - 1);
                view_scroll_to_cursor(v);
            }
            break;
        case KEY_TAB:
            {
                /* @cleanup: backspace is 4 */
                buffer_insert(b, offset, TAB);
                editor_set_cursor_from_offset(v, b, offset + TAB.len);
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
                buffer_insert(b, offset, text);
                editor_set_cursor_from_offset(v, b, offset + 1);
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

        if (E.pending_op == YANK)
        {
            if (E.pending_op_stage == 0)
            {
                if (c == INNER)
                {
                    E.pending_op_stage = 1;
                    return;
                }
                else if (c == 'w')
                {

                }
                else if (c == YANK)
                {
                    editor_free_register_one();
                    string *s = malloc(sizeof(string));
                    u64 line = v->cursor.y;
                    u64 start = buffer_line_start(b, line);
                    u64 line_len = buffer_line_len(b, line);
                    buffer_slice(b, start, line_len, s);

                    E.register_one = s;
                    E.paste_newline = TRUE;
                    editor_clear_pending_op();
                    return;
                }
            }

            if (E.pending_op_stage == 1)
            {
                if (c == WORD)
                {
                    editor_range range = editor_inner_word_range(
                            b,
                            editor_cursor_offset(v, b)
                            );

                    if (range.end > range.start)
                    {
                        editor_free_register_one();
                        string *s = (string*)malloc(sizeof(string));
                        buffer_slice(b, range.start, range.end - range.start, s);
                        E.register_one = s;
                        E.paste_newline = FALSE;
                        editor_set_cursor_from_offset(v, b, range.start);
                        editor_clamp_cursor_x(v, b);
                        view_scroll_to_cursor(v);
                    }

                    editor_clear_pending_op();
                    return;
                }
            }
        }

        if (E.pending_op == DELETE)
        {
            if (E.pending_op_stage == 0)
            {
                if (c == INNER)
                {
                    E.pending_op_stage = 1;
                    return;
                }
                else if (c == WORD)
                {
                    editor_range range = editor_inner_word_range(b, editor_cursor_offset(v, b));

                    if (range.end > range.start)
                    {
                        buffer_delete(b, range.start, range.end - range.start);
                        editor_set_cursor_from_offset(v, b, range.start);
                        editor_clamp_cursor_x(v, b);
                        view_scroll_to_cursor(v);
                    }

                    editor_clear_pending_op();
                    return;
                }

                editor_clear_pending_op();
                return;
            }

            if (E.pending_op_stage == 1)
            {
                if (c == WORD)
                {
                    editor_range range = editor_inner_word_range(b, editor_cursor_offset(v, b));

                    if (range.end > range.start)
                    {
                        buffer_delete(b, range.start, range.end - range.start);
                        editor_set_cursor_from_offset(v, b, range.start);
                        editor_clamp_cursor_x(v, b);
                        view_scroll_to_cursor(v);
                    }

                    editor_clear_pending_op();
                    return;
                }

                editor_clear_pending_op();
                return;
            }
        }

        if (E.pending_op == YANK)
        {

            return;
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

void
editor_at_exit()
{
    editor_free_register_one();
    write(STDOUT_FILENO, SHOW_CURSOR, SHOW_CURSOR_LEN);
    term_exit_alt_screen();
    term_disable_raw_mode(STDIN_FILENO);
}

void
editor_draw()
{
    view* v = &E.views[0];
    buffer *b = editor_active_buffer();
    int screen_row;
    u64 gutter_width = 2;

    if (b->lines.count > 0)
    {
        u64 line_count = b->lines.count;

        gutter_width = 0;
        while (line_count > 0)
        {
            gutter_width++;
            line_count /= 10;
        }

        gutter_width += 2;
    }

    write(STDOUT_FILENO, HIDE_CURSOR, HIDE_CURSOR_LEN);
    write(STDOUT_FILENO, CURSOR_HOME, CURSOR_HOME_LEN);

    for (screen_row = 0; screen_row < E.screenrows; screen_row++)
    {
        u64 line = v->rowoff + screen_row;
        u8 is_cursor_line = (line == v->cursor.y);

        if (is_cursor_line)
        {
            write(STDOUT_FILENO, CURSOR_LINE_BG, CURSOR_LINE_BG_LEN);
        }

        if (line >= b->lines.count)
        {
            u64 i;

            write(STDOUT_FILENO, "~", 1);
            for (i = 1; i < gutter_width; i++)
            {
                write(STDOUT_FILENO, " ", 1);
            }
        }
        else
        {
            u64 line_start = buffer_line_start(b, line);
            u64 line_len = buffer_line_len(b, line);
            u64 draw_start = v->coloff;
            u64 text_cols = 0;
            u64 draw_len = 0;
            u64 i;
            char ln[32];
            int ln_len;

            ln_len = snprintf(ln, sizeof(ln), "%*llu ",
                              (int)(gutter_width - 1),
                              (unsigned long long)(line + 1));
            if (ln_len > 0)
            {
                write(STDOUT_FILENO, ln, (size_t)ln_len);
            }

            if (E.screencols > (int)gutter_width)
            {
                text_cols = (u64)E.screencols - gutter_width;
            }

            if (draw_start < line_len && text_cols > 0)
            {
                draw_len = line_len - draw_start;
                if (draw_len > text_cols)
                {
                    draw_len = text_cols;
                }

                for (i = 0; i < draw_len; i++)
                {
                    u8 c = buffer_byte_at(b, line_start + draw_start + i);
                    write(STDOUT_FILENO, &c, 1);
                }
            }

            for (i = gutter_width + draw_len; i < (u64)E.screencols; i++)
            {
                write(STDOUT_FILENO, " ", 1);
            }
        }

        if (is_cursor_line)
        {
            write(STDOUT_FILENO, "\x1b[0m", 4);
        }

        if (screen_row < E.screenrows - 1)
        {
            write(STDOUT_FILENO, "\r\n", 2);
        }
    }

    /* status bar */
    write(STDOUT_FILENO, "\x1b[47;30m", 8); /* white bg, black fg */
    write(STDOUT_FILENO, NEXT_LINE, NEXT_LINE_LEN);
    write(STDOUT_FILENO, CLEAR_LINE, CLEAR_LINE_LEN);
    write(STDOUT_FILENO, b->file_path.s, b->file_path.len);
    write(STDOUT_FILENO, "\x1b[0m", 4);

    /* command bar */
    write(STDOUT_FILENO, NEXT_LINE, NEXT_LINE_LEN);
    if (E.mode == EDITOR_COMMAND_MODE && E.cmd.cur_pos > 0)
    {
        write(STDOUT_FILENO, CLEAR_LINE, CLEAR_LINE_LEN);
        write(STDOUT_FILENO, E.cmd.data, E.cmd.cur_pos);
        write(STDOUT_FILENO, SHOW_CURSOR, SHOW_CURSOR_LEN);
        return;
    }
    else if (E.status_message[0] != '\0')
    {
        write(STDOUT_FILENO, E.status_message, strlen((char *)E.status_message));
    }
    else
    {
        write(STDOUT_FILENO, CLEAR_LINE, CLEAR_LINE_LEN);
    }

    {
        char seq[32];
        u64 cursor_screen_x = 0;
        int cursor_row = (int)(v->cursor.y - v->rowoff) + 1;

        if (v->cursor.x > v->coloff)
        {
            cursor_screen_x = v->cursor.x - v->coloff;
        }

        int cursor_col = (int)(gutter_width + cursor_screen_x + 1);
        snprintf(seq, sizeof(seq), SET_CURSOR_POS, cursor_row, cursor_col);
        write(STDOUT_FILENO, seq, strlen(seq));
    }

    if (E.mode == EDITOR_PENDING_OP_MODE)
    {
        write(STDOUT_FILENO, UNDERLINE_CURSOR, UNDERLINE_CURSOR_LEN);
    }
    else
    {
        write(STDOUT_FILENO, BOX_CURSOR, BOX_CURSOR_LEN);
    }

    write(STDOUT_FILENO, SHOW_CURSOR, SHOW_CURSOR_LEN);
}


void editor_init(void)
{
    E.mode = EDITOR_NORMAL_MODE;
    E.running = 1;
    E.alt_screen = 0;
    E.scratch = new_arena(MB(1));
    E.cmd = new_arena(MB(1));
    E.status_message[0] = '\0';

    buffer* buffers = (buffer*)malloc(sizeof(buffer)*32);
    if (buffers == NULL)
    {
        perror("[error] unable to allocate memory for buffers");
        exit(1);
    }
    E.buffers = buffers;

    const char* p = "/home/failbot/src/editor/data/cpu.c";
    string path = {.s = (u8*)p, .len = strlen(p)};
    string file;
    readfile(p, &file, MAX_FILE_SIZE);

    buffer_init(&E.buffers[0], file, path);

    view* views = (view*)malloc(sizeof(view)*32);
    if (views == NULL)
    {
        perror("[error] unable to allocate memory for buffers");
        exit(1);
    }
    E.views = views;

    E.views[0].buffer_id = 0;
    E.views[0].cursor.x = 0;
    E.views[0].cursor.y = 0;
    E.views[0].rowoff = 0;
    E.views[0].coloff = 0;
    E.pending_op = 0;
    E.pending_op_stage = 0;

    /* @cleanup tmp */
    E.active_view = 0;

    term_update_window_size();
    term_install_signal_handlers();
}
