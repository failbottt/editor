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
#define EDITOR_PENDING_OP_MODE 5
#define EDITOR_SEARCH_MODE 6

typedef struct {
    u64 mode;
    u64 running;
    u64 screenrows;
    u64 screencols;
    u64 rawmode;
    u64 alt_screen;
    u64 pending_op;
    u64 pending_op_stage;

    buffer* buffers;
    view* views;

    arena scratch;

    /* @cleanup */
    arena cmd;
    u8 status_message[256];

} editor;

extern editor E;

#define TAB (string){.s = (u8*)"    ", .len = 4}

u64 editor_read_key(int fd);
void editor_process_keypress(int c);
void editor_move_cursor(u64 c);
void editor_set_cmd_status_message(u8 *msg);
buffer* editor_active_buffer();
void editor_at_exit();
void editor_draw();
void editor_init();

#endif
