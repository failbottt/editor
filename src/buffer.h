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
    u64 *line_starts;
    u64 line_count;
    u64 capacity;
} line_cache ;

typedef struct
{
    string original;
    arena add;
    string file_path;

    piece_list *list;

    u64 version;
    u64 dirty;

    line_cache lines;
    /* undo */
    /* redo */
} buffer;

buffer buffer_init(string path, string contents);
void buffer_destroy(buffer *b);
void buffer_build_line_cache(buffer *b);
void buffer_insert(buffer *b, u64 pos, string s);
void buffer_delete(buffer *b, u64 start, u64 end);

#endif

/*  buffer_t *buffer_load(path, contents);
  int buffer_save(buffer_t *, path);

  buffer_version_t buffer_version(buffer_t *);

  buffer_range_t buffer_replace(buffer_t *, buffer_pos_t start,
  buffer_pos_t end, string text);
  buffer_range_t buffer_insert(buffer_t *, buffer_pos_t pos,
  string text);
  buffer_range_t buffer_delete(buffer_t *, buffer_pos_t start,
  buffer_pos_t end);

  string_view buffer_slice(buffer_t *, buffer_pos_t start,
  buffer_pos_t end);

  buffer_pos_t buffer_offset_to_pos(buffer_t *, size_t offset);
  size_t buffer_pos_to_offset(buffer_t *, buffer_pos_t pos);

  size_t buffer_len(buffer_t *);
  size_t buffer_line_count(buffer_t *);

  bool buffer_undo(buffer_t *);
  bool buffer_redo(buffer_t *); */
