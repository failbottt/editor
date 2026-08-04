#include <unistd.h>

#include "term.h"
#include "editor.h"

void
terminal_enter_alt_screen()
{
    if (E.alt_screen) return;
    write(STDOUT_FILENO, ENTER_ALT_SCREEN, ENTER_ALT_SCREEN_LEN);
    E.alt_screen = 1;
}

void terminal_exit_alt_screen(void)
{
    if (!E.alt_screen) return;
    write(STDOUT_FILENO, LEAVE_ALT_SCREEN, LEAVE_ALT_SCREEN_LEN);
    E.alt_screen = 0;
}
