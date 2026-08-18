#ifndef TERM_H
#define TERM_H

#include "base.h"

u64  term_enable_raw_mode(int fd);
u64  term_get_cursor_position(u64 ifd, u64 ofd, u64 *rows, u64 *cols);
u64  term_get_window_size(u64 ifd, u64 ofd, u64 *rows, u64 *cols);
void term_enter_alt_screen();
void term_exit_alt_screen();
void term_disable_raw_mode(u64 fd);
void term_handle_sigwinch(int unused __attribute__((unused)));
void term_handle_termination_signal(int signum);
void term_install_signal_handlers();
void term_update_window_size();

#define CURSOR_HOME            "\x1b[H"
#define CURSOR_HOME_LEN        3

#define CLEAR_SCREEN           "\x1b[2J"
#define CLEAR_SCREEN_LEN       4

#define CLEAR_LINE             "\x1b[K"
#define CLEAR_LINE_LEN         3

#define CLEAR_TO_END_SCREEN    "\x1b[J"
#define CLEAR_TO_END_SCREEN_LEN 3

#define CLEAR_WHOLE_LINE       "\x1b[2K"
#define CLEAR_WHOLE_LINE_LEN   4

#define HIDE_CURSOR            "\x1b[?25l"
#define HIDE_CURSOR_LEN         6

#define SHOW_CURSOR            "\x1b[?25h"
#define SHOW_CURSOR_LEN         6

#define ENTER_ALT_SCREEN       "\x1b[?1049h"
#define ENTER_ALT_SCREEN_LEN    8

#define LEAVE_ALT_SCREEN       "\x1b[?1049l"
#define LEAVE_ALT_SCREEN_LEN    8

#define NEXT_LINE               "\x1b[E"
#define NEXT_LINE_LEN           3

#define SET_CURSOR_POS          "\x1b[%d;%dH"

#define UNDERLINE_CURSOR        "\x1b[4 q"
#define UNDERLINE_CURSOR_LEN    5

#define BOX_CURSOR              "\x1b[2 q"
#define BOX_CURSOR_LEN          5

#define CURSOR_LINE_BG          "\x1b[48;5;235m"
#define CURSOR_LINE_BG_LEN      11


#endif
