#ifndef BUFFER_H
#define BUFFER_H

#include "base.h"

typedef enum {
    BUFFER_SRC_ORIG,
    BUFFER_SRC_ADD,
} buffer_source;

typedef struct
{
    u64 piece_index;
    u64 piece_offset;
    u64 doc_start;
} piece_loc;

typedef struct
{
    buffer_source source;
    u64 start;
    u64 len;
} piece;

typedef struct
{
    piece* items;
    u64 count;
    u64 capacity;
} piece_array;

typedef struct
{
    u64 start;
} line_info;

typedef struct
{
    line_info *items;
    u64 count;
    u64 capacity;
} line_index;

typedef struct
{
    string file_path;

    string orig;
    string add;
    u64 add_capacity;
    piece_array pieces;

    line_index lines;
    u64 total_len;
} buffer;

void buffer_init(buffer *b, string data, string path);
void buffer_insert(buffer *b, u64 offset, string text);
void buffer_delete(buffer *b, u64 start, u64 len);
u8 buffer_byte_at(buffer *b, u64 offset);
void buffer_slice(buffer *b, u64 start, u64 len, string *out);
u64 buffer_line_start(buffer *b, u64 line);
u64 buffer_line_len(buffer *b, u64 line);
u64 buffer_offset_to_line_col(buffer *b, u64 offset, u64 *line, u64 *col);
u64 buffer_line_col_to_offset(buffer *b, u64 line, u64 col);
string buffer_to_string(buffer *b);




#endif
