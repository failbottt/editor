#include <termios.h>
#include <unistd.h>
#include <stdio.h>

#include "base.h"

#include "input.h"
#include "term.h"
#include "buffer.h"
#include "editor.h"
#include "gfx.h"
#include "view.h"

editor E = (editor){0};

int main(int argc, char **argv)
{
    gfx_init();

    string p = {.data = (u8*)"foo/bar.txt", .len = 11};

    u8 *d = (u8*)"foo1barbaz\nfoo2hello\no3\nfoo4-----------------";
    string code = {.data = d, .len = strlen(d)};

    buffer b = buffer_init(p, code);

    E.mode = NORMAL;
    E.running = 1;
    E.cursor_offset = 0;

    buffer *buffers = (buffer *)malloc(sizeof(buffer)*1);
    if (buffers == NULL)
    {
        fprintf(stderr, "[error]: Unable to malloc buffers");
        exit(1);
    }
    E.buffers = buffers;

    E.buffers[0] = b;

    E.active_buffer = &E.buffers[0];

    view *views = (view*)malloc(sizeof(*views)*1);
    if (views == NULL)
    {
        fprintf(stderr, "[error]: Unable to malloc views");
        exit(1);
    }
    E.views = views;

    E.views[0].b = E.active_buffer;


    while (E.running)
    {
        term_cursor_to_home();
        gfx_clear_screen();

        term_hide_cursor();

        view *current_view = &E.views[0];

        view_draw(current_view);

        cursor_pos cursor = buffer_offset_to_screen_pos(
                current_view->b,
                E.cursor_offset
                );

        term_set_cursor_pos(cursor.x, cursor.y);

        if (E.mode == INSERT || E.mode == INSERT_RIGHT_OF_CURSOR)
        {
            term_cursor_as_line();
        }
        else
        {
            term_cursor_as_block();
        }

        term_show_cursor();

        key k = input_get_key();

        editor_process_input(k);

    }

    gfx_cleanup();
    buffer_destroy(&E.buffers[0]);
    free(E.buffers);

    return 0;
}
