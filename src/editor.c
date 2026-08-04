#include "editor.h"
#include "keys.h"
#include "buffer.h"
#include "cmd.h"

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
    u64 line_len = buffer_line_len(b, v->cursor.y) - 1;
    if (v->cursor.x > line_len)
    {
        v->cursor.x = line_len;
    }
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
editor_process_keypress(int fd) {
    int c = editor_read_key(fd);
    view *v = &E.views[0];
    buffer *buf = &E.buffers[v->buffer_id];

    if (E.mode == EDITOR_NORMAL_MODE)
    {
        switch(c) {
        case KEY_J:
        case KEY_K:
        case KEY_H:
        case KEY_L:
            editor_move_cursor(c);
            break;
        case 'i':
            E.mode = EDITOR_INSERT_MODE;
            break;
        case ':':
            E.mode = EDITOR_COMMAND_MODE;
            u8 *colon = (u8*)":";
            arena_push_array(&E.cmd, colon, 1);
            break;
        case ESC:
            exit(0);
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
    }

    if (E.mode == EDITOR_VISUAL_MODE)
    {

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
                process_cmd(E.cmd);
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
