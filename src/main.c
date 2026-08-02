#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <X11/keysym.h>

#include "base.h"
#include "editor.h"
#include "keys.h"
#include "buffer.h"
#include "cursor.h"

/* define globally */
editor E;

static struct termios orig_termios; /* In order to restore at exit.*/

void disable_raw_mode(int fd) {
    /* Don't even check the return value as it's too latE. */
    if (E.rawmode) {
        tcsetattr(fd,TCSAFLUSH,&orig_termios);
        E.rawmode = 0;
    }
}

/* Called at exit to avoid remaining in raw modE. */
void editor_at_exit(void) {
    disable_raw_mode(STDIN_FILENO);
}

/* Raw mode: 1960 magic shit. */
int enable_raw_mode(int fd) {
    struct termios raw;

    if (E.rawmode) return 0; /* Already enabled. */
    if (!isatty(STDIN_FILENO)) goto fatal;
    atexit(editor_at_exit); if (tcgetattr(fd,&orig_termios) == -1) goto fatal;

    raw = orig_termios;  /* modify the original mode */
    /* input modes: no break, no CR to NL, no parity check, no strip char,
     * no start/stop output control. */
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    /* output modes - disable post processing */
    raw.c_oflag &= ~(OPOST);
    /* control modes - set 8 bit chars */
    raw.c_cflag |= (CS8);
    /* local modes - choing off, canonical off, no extended functions,
     * no signal chars (^Z,^C) */
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    /* control chars - set return condition: min number of bytes and timer. */
    raw.c_cc[VMIN] = 0; /* Return each byte, or zero for timeout. */
    raw.c_cc[VTIME] = 1; /* 100 ms timeout (unit is tens of second). */

    /* put terminal in raw mode after flushing */
    if (tcsetattr(fd,TCSAFLUSH,&raw) < 0) goto fatal;
    E.rawmode = 1;
    return 0;

fatal:
    errno = ENOTTY;
    return -1;
}


/* Use the esc [6n escape sequence to query the horizontal cursor position
 * and return it. On error -1 is returned, on success the position of the
 * cursor is stored at *rows and *cols and 0 is returned. */
int get_cursor_position(int ifd, int ofd, int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    /* Report cursor location */
    if (write(ofd, "\x1b[6n", 4) != 4) return -1;

    /* Read the response: esc [ rows ; cols R */
    while (i < sizeof(buf)-1) {
        if (read(ifd,buf+i,1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    /* Parse it. */
    if (buf[0] != ESC || buf[1] != '[') return -1;
    if (sscanf(buf+2,"%d;%d",rows,cols) != 2) return -1;
    return 0;
}

/* Try to get the number of columns in the current terminal. If the ioctl()
 * call fails the function will try to query the terminal itself.
 * Returns 0 on success, -1 on error. */
int get_window_size(int ifd, int ofd, int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* ioctl() failed. Try to query the terminal itself. */
        int orig_row, orig_col, retval;

        /* Get the initial position so we can restore it later. */
        retval = get_cursor_position(ifd,ofd,&orig_row,&orig_col);
        if (retval == -1) goto failed;

        /* Go to right/bottom margin and get position. */
        if (write(ofd,"\x1b[999C\x1b[999B",12) != 12) goto failed;
        retval = get_cursor_position(ifd,ofd,rows,cols);
        if (retval == -1) goto failed;

        /* Restore position. */
        char seq[32];
        snprintf(seq,32,"\x1b[%d;%dH",orig_row,orig_col);
        if (write(ofd,seq,strlen(seq)) == -1) {
            /* Can't recover... */
        }
        return 0;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }

failed:
    return -1;
}

int editor_file_was_modified(void)
{
    return E.dirty;
}

void update_window_size(void)
{
    if (get_window_size(STDIN_FILENO,STDOUT_FILENO,
                      &E.screenrows,&E.screencols) == -1) {
        perror("Unable to query the screen for size (columns / rows)");
        exit(1);
    }
    E.screenrows -= 2; /* Get room for status bar. */
}

void editor_draw()
{
    view *v = &E.views[0];
    buffer *b = &E.buffers[v->buffer_id];

    /* clear and home */
    write(STDOUT_FILENO, "\x1b[H\x1b[J", 6);

    if (b->orig_text.len > 0)
    {
        u64 i;
        u64 rows_drawn, docs_row = 0;
        for  (i = 0; i < b->orig_text.len; i++)
        {
            if (rows_drawn == E.screenrows)
            {
                break;
            }
            if (b->orig_text.s[i] == '\n')
            {
                /* next line */
                write(STDOUT_FILENO, "\x1b[E", 3);
                /* clear from cursor to end */
                write(STDOUT_FILENO, "\x1b[K", 3);
                rows_drawn++;
            }
            else
            {
                write(STDOUT_FILENO, &b->orig_text.s[i], 1);
            }
        }
    }

    char cur_pos[7];
    snprintf(cur_pos, sizeof(cur_pos), "\x1b[%d;%dH", (int)v->cursor.y, (int)v->cursor.x);
    write(STDOUT_FILENO, cur_pos, 7);

    /* cursor top left for now */
    /*write(STDOUT_FILENO, "\x1b[H", 3);*/
}

void handle_sigwinch(int unused __attribute__((unused))) {
    update_window_size();
    /*if (E.cy > E.screenrows) E.cy = E.screenrows - 1;
    if (E.cx > E.screencols) E.cx = E.screencols - 1;*/
    editor_draw();
}

void init_editor(void) {
    E.mode = EDITOR_NORMAL_MODE;
    E.numrows = 0;
    E.dirty = 0;
    E.filename = NULL;
    E.syntax = NULL;

    buffer* buffers = (buffer*)malloc(sizeof(buffer)*32);
    if (buffers == NULL)
    {
        perror("[init] unable to malloc buffers");
        exit(1);
    }
    E.buffers = buffers;

    string tmp = {0};
    s64 read = readfile("/home/failbot/src/editor/cpu.c", &tmp, MAX_FILE_SIZE);
    if (read < 0)
    {
        perror("[init] unable to read file");
        exit(errno);
    }

    E.buffers[0].orig_text.len = tmp.len;
    E.buffers[0].orig_text.s = tmp.s;

    view* views = (view*)malloc(sizeof(view)*1);
    if (views == NULL)
    {
        perror("[init] unable to malloc views");
        exit(1);
    }
    E.views = views;

    /* @cleanup */
    view *v = &views[0];
    v->buffer_id = 0;
    v->cursor.x = 0;
    v->cursor.y = 0;
    v->rowoff = 0;
    v->coloff = 0;

    update_window_size();
    signal(SIGWINCH, handle_sigwinch);
}

int main(int argc, char **argv)
{
    init_editor();
    enable_raw_mode(STDIN_FILENO);
    while(1)
    {
        editor_draw();
        editor_process_keypress(STDIN_FILENO);
    }
    return 0;
}
