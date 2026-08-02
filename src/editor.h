#ifndef EDITOR_H
#define EDITOR_H

#include <time.h>

#include "base.h"
#include "view.h"
#include "buffer.h"

#define MAX_FILE_SIZE GB(1)

#define EDITOR_NORMAL_MODE 1
#define EDITOR_INSERT_MODE 2
#define EDITOR_VISUAL_MODE 3

typedef struct {
    int mode;
    int screenrows; /* Number of rows that we can show */
    int screencols; /* Number of cols that we can show */
    int numrows;    /* Number of rows */
    int rawmode;    /* Is terminal raw mode enabled? */
    int dirty;      /* File modified but not saved. */
    char *filename; /* Currently open filename */
    char statusmsg[80];
    time_t statusmsg_time;
    struct editor_syntax *syntax;    /* Current syntax highlight, or NULL. */

    buffer* buffers;
    view* views;

} editor;

extern editor E;

void editor_process_keypress(int fd);
void editor_move_cursor(u64 c);
u64 editor_read_key(int fd);
#endif
