#ifndef EDITOR_H
#define EDITOR_H

#include "base.h"
#include "input.h"

typedef struct buffer buffer;
typedef struct view view;

typedef enum
{
    EMPTY = 0,
    NORMAL,
    INSERT,
    INSERT_RIGHT_OF_CURSOR,
    VISUAL,
} editor_mode;

typedef struct
{
    u64 running;
    editor_mode mode;

    view *views;

    buffer *buffers;
    buffer *active_buffer;

    u64 cursor_offset;
} editor;

extern editor E;

void editor_init(string file_path);
void editor_process_input(key k);
void editor_process_normal_mode_key(key k);
void editor_process_insert_mode_key(key k);
void editor_process_visual_mode_key(key k);

#endif
