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
#include "term.h"

/* define globally */
editor E;

static struct termios orig_termios; /* In order to restore at exit.*/

void editor_at_exit(void);
void handle_sigwinch(int unused __attribute__((unused)));

static void handle_termination_signal(int signum) {
    editor_at_exit();
    signal(signum, SIG_DFL);
    raise(signum);
}

static void install_signal_handlers(void) {
    int signals[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
    struct sigaction sa;
    size_t i;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_termination_signal;
    sigemptyset(&sa.sa_mask);

    for (i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
        sigaction(signals[i], &sa, NULL);
    }

    signal(SIGWINCH, handle_sigwinch);
}

void disable_raw_mode(int fd) {
    if (E.rawmode) {
        tcsetattr(fd,TCSAFLUSH,&orig_termios);
        E.rawmode = 0;
    }
}

void editor_at_exit(void)
{
    write(STDOUT_FILENO, SHOW_CURSOR, SHOW_CURSOR_LEN);
    terminal_exit_alt_screen();
    disable_raw_mode(STDIN_FILENO);
}

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

void update_window_size(void)
{
    if (get_window_size(STDIN_FILENO,STDOUT_FILENO,
                      &E.screenrows,&E.screencols) == -1) {
        perror("[error] unable to query the screen for size (columns / rows)");
        exit(1);
    }

    E.screenrows -= 2;
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


void handle_sigwinch(int unused __attribute__((unused)))
{
    update_window_size();
    editor_draw();
}

void init_editor(void)
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

    update_window_size();
    install_signal_handlers();
}

int main(int argc, char **argv)
{
    init_editor();

    terminal_enter_alt_screen();
    enable_raw_mode(STDIN_FILENO);

    while(E.running)
    {
        editor_draw();

        int c = editor_read_key(STDIN_FILENO);
        editor_process_keypress(c);

        /* end of frame cleanup */
        E.scratch.cur_pos = 0;
    }

    /* cleanup / shutdown */

    return 0;
}
