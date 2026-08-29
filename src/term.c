#include <unistd.h>

#include "term.h"

#define CLEAR_SCREEN  "\x1b[2J"
#define CLEAR_SCREEN_LEN  4

#define CURSOR_TO_HOME "\x1b[H"
#define CURSOR_TO_HOME_LEN 3

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
