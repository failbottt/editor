#include <unistd.h>
#include <string.h>

#include "base.h"
#include "buffer.h"
#include "gfx.h"
#include "input.h"

struct foo
{
    int x;
};

static void test_cursor_and_input()
{
    gfx_init();
    u64 x = 1;
    u64 y = 1;

    while(1)
    {
        key k = input_get_key();
        if (k.value == KEY_ESCAPE)
        {
            break;
        }
        else if (k.value == KEY_RETURN)
        {
            x = 1;
            y++;
            term_set_cursor_pos(x, y);
            continue;
        }

        term_set_cursor_pos(x, y);

        gfx_draw_text("A", 1);
        x++;
    }

    gfx_cleanup();
}

int main()
{
    /* test_cursor_and_input(); */

    struct foo foo = {0};

    return 0;
}

