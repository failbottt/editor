#ifndef EDITOR_H
#define EDITOR_H

#include "base.h"
#include "view.h"
#include "buffer.h"

#define MAX_FILE_SIZE GB(1)

#define EDITOR_NORMAL_MODE  1
#define EDITOR_INSERT_MODE  2
#define EDITOR_VISUAL_MODE  3
#define EDITOR_COMMAND_MODE 4

typedef struct {
    int mode;
    int running;
    int screenrows;
    int screencols;
    int rawmode;
    int alt_screen;

    buffer* buffers;
    view* views;

    arena scratch;

    /* @cleanup */
    arena cmd;
} editor;

extern editor E;

void editor_process_keypress(int fd);
void editor_move_cursor(u64 c);
u64 editor_read_key(int fd);
#endif
