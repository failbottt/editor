#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include "input.h"
#include "term.h"
#include "buffer.h"

int main(int argc, char **argv)
{
    struct termios orig = {0};
    struct termios raw = {0};

    tcgetattr(STDIN_FILENO, &orig);
    tcgetattr(STDIN_FILENO, &raw);
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    term_enter_alt_screen();
    term_clear_screen();
    term_cursor_to_home();

    int running = 1;
    int n;

    string p = {.s = (u8*)"foo/bar.txt", .len = 11};
    string code = {.s = (u8*)"hello\nworld\n", .len = 12};

    buffer b = buffer_init(p, code);

    buffer_insert(&b, 0, p);

    while (running)
    {
        key k = getkey();

        if (k.value == KEY_ESCAPE)
        {
            running = 0;
        }

        char b2[128];
        sprintf(b2, "%c", k.value);
        write(STDOUT_FILENO, b2, 1);
    }
    write(STDOUT_FILENO, "\x1b[0m", 4);

    /* reset to original terminal output */
    term_leave_alt_screen();

    tcsetattr(STDIN_FILENO, TCSANOW, &orig);

    return 0;
}
