#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include <termios.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <fcntl.h>
#include <X11/keysym.h>

#include "base.h"
#include "editor.h"
#include "term.h"

/* define globally */
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
