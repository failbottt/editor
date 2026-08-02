#include "editor.h"
#include "keys.h"

/* Read a key from the terminal put in raw mode, trying to handle
 * escape sequences. */
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

static void
view_scroll_to_cursor(view *v)
{

}

void
editor_move_cursor(u64 key)
{
    view *v = &E.views[0];
    switch (key) {
        case KEY_J:
            v->cursor.y++;
            break;
        case KEY_K:
            if (v->cursor.y > 0) v->cursor.y--;
            break;
        case KEY_L:
            v->cursor.x++;
            break;
        case KEY_H:
            if (v->cursor.x > 0) v->cursor.x--;
            break;
    }
}

/* Handle cursor position change because arrow keys were pressed. */
/* Process events arriving from the standard input, which is, the user
 * is typing stuff on the terminal. */
void
editor_process_keypress(int fd) {
    int c = editor_read_key(fd);

    char b[32] = "";
    sprintf(b, "%d", c);

    if (E.mode == EDITOR_NORMAL_MODE)
    {
        switch(c) {
        case KEY_J:
        case KEY_K:
        case KEY_H:
        case KEY_L:
            editor_move_cursor(c);
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
    }

    if (E.mode == EDITOR_VISUAL_MODE)
    {

    }
}


