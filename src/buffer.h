#ifndef BUFFER_H
#define BUFFER_H

#include "base.h"

typedef struct
{
    u8* source;
    u64 start;
    u64 len;
} piece;

typedef struct
{
    u64 doc_row;
    u64 doc_col;
    string file_path;
    string orig_text;
    string add;
    piece* pieces;
} buffer;

#endif
