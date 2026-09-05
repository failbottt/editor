#include <termios.h>
#include <unistd.h>

#include "base.h"
#include "gfx.h"
#include "term.h"

struct termios orig = {0};
struct termios raw = {0};

void gfx_init()
{
    tcgetattr(STDIN_FILENO, &orig);
    tcgetattr(STDIN_FILENO, &raw);
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    term_enter_alt_screen();
    term_clear_screen();
    term_cursor_to_home();
}

void gfx_clear_screen()
{
    term_clear_screen();
}

void gfx_draw_text(u8 *text, u64 len)
{
    write(STDOUT_FILENO, text , len);
}

void gfx_cleanup()
{
    /* reset to original terminal output */
    term_leave_alt_screen();
    tcsetattr(STDIN_FILENO, TCSANOW, &orig);
}
