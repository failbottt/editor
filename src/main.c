#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include "input.h"
#include "term.h"
#include "buffer.h"
#include "editor.h"
#include "gfx.h"
#include "view.h"

int main(int argc, char **argv)
{
    gfx_init();

    string p = {.data = (u8*)"foo/bar.txt", .len = 11};
    string code = {.data = (u8*)"helloworld", .len = 10};

    buffer b = buffer_init(p, code);

    editor E = (editor){0};
    E.running = 1;
    E.buffers = (buffer *)malloc(sizeof(buffer)*2);
    E.buffers[0] = b;
    E.current_buffer = 0;

    u64 cursor = b.list->pieces[0].len;
    while (E.running)
    {
        term_cursor_to_home();
        gfx_clear_screen();

        buffer *current_buffer = &E.buffers[E.current_buffer];

        view_draw(current_buffer);
        key k = getkey();

        if (k.value == KEY_ESCAPE)
        {
            E.running = 0;
            break;
        }

        u8 b2[128];
        sprintf((char *)b2, "%c", k.value);

        string s_tmp = {.data = b2, .len = 1};
        buffer_insert(current_buffer, cursor, s_tmp);
        cursor++;
    }

    gfx_draw_text((u8*)"\x1b[0m", 4);
    gfx_cleanup();

    buffer_destroy(&E.buffers[0]);
    free(E.buffers);

    return 0;
}
