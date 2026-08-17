#ifndef FILE_H
#define FILE_H

#include "buffer.h"

typedef enum
{
    WRITE_FILE_OK = 0,
    WRITE_FILE_NO_PATH,
    WRITE_FILE_NEEDS_CONFIRMATION,
    WRITE_FILE_OPEN_FAILED,
    WRITE_FILE_WRITE_FAILED,
    WRITE_FILE_RENAME_FAILED,
    WRITE_FILE_STAT_FAILED
} write_file_status;

typedef struct
{
    write_file_status status;
    string path;
    u64 bytes_written;
    u64 line_count;
    struct stat written_stat;
} write_file_result;

write_file_result write_file(buffer *b, int force);

#endif
