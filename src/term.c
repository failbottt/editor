#include <unistd.h>

#include "base.h"
#include "term.h"

#define CLEAR_SCREEN  "\x1b[2J"
#define CLEAR_SCREEN_LEN  4

#define CURSOR_TO_HOME "\x1b[H"
#define CURSOR_TO_HOME_LEN 3

#define CURSOR_HIDE "\x1b[?25l"
#define CURSOR_HIDE_LEN 6

#define CURSOR_SHOW "\x1b[?25h"
#define CURSOR_SHOW_LEN 6

/* dynamic so it has no length */
#define CURSOR_SET_POS "\x1b[%d;%dH"

#define ENTER_ATL_SCREEN "\x1b[?1049h"
#define ENTER_ATL_SCREEN_LEN 8

#define LEAVE_ATL_SCREEN "\x1b[?1049l"
#define LEAVE_ATL_SCREEN_LEN 8



void term_clear_screen()
{
    write(
            STDOUT_FILENO,
            CLEAR_SCREEN,
            CLEAR_SCREEN_LEN
         );
}

void term_cursor_to_home()
{
    write(
            STDOUT_FILENO,
            CURSOR_TO_HOME,
            CURSOR_TO_HOME_LEN
         );
}

void term_enter_alt_screen()
{
    write(
            STDOUT_FILENO,
            ENTER_ATL_SCREEN,
            ENTER_ATL_SCREEN_LEN
         );
}

void term_leave_alt_screen()
{
    write(
            STDOUT_FILENO,
            LEAVE_ATL_SCREEN,
            LEAVE_ATL_SCREEN_LEN
         );
}

void term_hide_cursor()
{
    write(
            STDOUT_FILENO,
            CURSOR_HIDE,
            CURSOR_HIDE_LEN
         );
}

void term_show_cursor()
{
    write(
            STDOUT_FILENO,
            CURSOR_SHOW,
            CURSOR_SHOW_LEN
         );
}

void term_set_cursor_pos(u64 x, u64 y)
{
    /* @note: terminal is row, col and 1, 1 based not 0,0 */
    /*x++;
    y++;*/

    if (y <= 0)
    {
        y = 1;
    }

    if (x <= 0)
    {
        x = 1;
    }

    /* @note: two 64 bit numbers is up to 40 digits */
    u8 b[64];

    /* @note: it's y,x not x,y */
    u64 len = snprintf(b, sizeof(b), CURSOR_SET_POS, y, x);

    /* @cleanup: maybe assert and crash? */
    if (len < 0 || len >= sizeof(b))
    {
        return;
    }

    write(STDOUT_FILENO, b, len);
}
