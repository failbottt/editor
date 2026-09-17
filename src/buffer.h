#ifndef BUFFER_H
#define BUFFER_H

#include "base.h"

typedef enum
{
    ADD,
    ORIGINAL
} source_type;

typedef struct
{
    u64 start;
    u64 len;
    source_type source;
} piece;

typedef struct
{
    u64 len;
    u64 capacity;
    piece *pieces;
} piece_list;

typedef struct
{
    u64 *offsets;
    u64 len;
    u64 capacity;
} line_cache ;

struct line
{
    u64 start;
    u64 end;
    u64 len;
};

typedef struct
{
    u8 found;
    u8 at_end;
    u64 index;
    u64 split_at;
} piece_hit;

typedef struct buffer
{
    string original;
    arena add;
    string file_path;

    piece_list *list;

    u64 version;
    u64 dirty;

    line_cache cached_line_starts;
    /* undo */
    /* redo */
} buffer;

buffer buffer_init(string path, string contents);
void buffer_destroy(buffer *b);
void buffer_build_line_cache(buffer *b);
void buffer_insert(buffer *b, u64 pos, string s);
void buffer_delete(buffer *b, u64 start, u64 end);
cursor_pos buffer_offset_to_screen_pos(buffer *b, u64 offset);
u64 buffer_document_length(buffer *b);
struct line buffer_line_length(buffer *b, u64 offset);

#endif
