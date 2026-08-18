#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>


#include "term.h"
#include "editor.h"
#include "base.h"
#include "keys.h"

/* In order to restore terminal at exit.*/
static struct termios orig_termios;

void
term_enter_alt_screen()
{
    if (E.alt_screen) return;
    write(STDOUT_FILENO, ENTER_ALT_SCREEN, ENTER_ALT_SCREEN_LEN);
    E.alt_screen = 1;
}

void
term_exit_alt_screen()
{
    if (!E.alt_screen) return;
    write(STDOUT_FILENO, LEAVE_ALT_SCREEN, LEAVE_ALT_SCREEN_LEN);
    E.alt_screen = 0;
}

void
term_disable_raw_mode(u64 fd)
{
    if (E.rawmode) {
        tcsetattr(fd, TCSAFLUSH, &orig_termios);
        E.rawmode = 0;
    }
}

void
term_update_window_size()
{
    u64 ws = term_get_window_size(
            STDIN_FILENO,
            STDOUT_FILENO,
            &E.screenrows,
            &E.screencols
            );

    if (ws == -1) {
        perror("[error] unable to query the screen for size (columns / rows)");
        exit(1);
    }

    E.screenrows -= 2;
}


u64
term_get_window_size(u64 ifd, u64 ofd, u64 *rows, u64 *cols)
{
    struct winsize ws;

    if (ioctl(1, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        /* ioctl() failed. Try to query the terminal itself. */
        u64 orig_row, orig_col, retval;

        /* Get the initial position so we can restore it later. */
        retval = term_get_cursor_position(ifd, ofd, &orig_row, &orig_col);
        if (retval == -1) goto failed;

        /* Go to right/bottom margin and get position. */
        if (write(ofd,"\x1b[999C\x1b[999B",12) != 12) goto failed;
        retval = term_get_cursor_position(ifd, ofd, rows, cols);
        if (retval == -1) goto failed;

        /* Restore position. */
        char seq[32];
        snprintf(seq,32,"\x1b[%d;%dH", (int)orig_row,(int)orig_col);
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

u64
term_get_cursor_position(u64 ifd, u64 ofd, u64 *rows, u64 *cols)
{
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
    if (sscanf(buf+2,"%d;%d",(int*)rows,(int*)cols) != 2) return -1;
    return 0;
}


u64
term_enable_raw_mode(int fd)
{
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

void
term_handle_sigwinch(int unused __attribute__((unused)))
{
    term_update_window_size();
    editor_draw();
}

void
term_handle_termination_signal(int signum)
{
    editor_at_exit();
    signal(signum, SIG_DFL);
    raise(signum);
}

void
term_install_signal_handlers(void)
{
    int signals[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
    struct sigaction sa;
    size_t i;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = term_handle_termination_signal;
    sigemptyset(&sa.sa_mask);

    for (i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
        sigaction(signals[i], &sa, NULL);
    }

    signal(SIGWINCH, term_handle_sigwinch);
}
