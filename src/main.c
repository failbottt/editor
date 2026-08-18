#include "editor.h"
#include "term.h"

editor E;

int main(int argc, char **argv)
{
    editor_init();

    term_enter_alt_screen();
    term_enable_raw_mode(STDIN_FILENO);

    while(E.running)
    {
        editor_draw();

        int c = editor_read_key(STDIN_FILENO);
        editor_process_keypress(c);

        /* end of frame cleanup */
        E.scratch.cur_pos = 0;
    }

    /* cleanup / shutdown */

    return 0;
}
