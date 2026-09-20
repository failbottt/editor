#include <unistd.h>
#include <string.h>

#include "base.h"
#include "buffer.h"
#include "gfx.h"
#include "input.h"

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

    buffer b = buffer_init(STR("/path/to/file"), STR("foo bar"));

    fprintf(stdout, "%c\n", buffer_char_at_offset(&b, 2));

    return 0;
}
