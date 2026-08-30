#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "base.h"
#include "buffer.h"

void buffer_insert(buffer *b, int pos, string s)
{
    if (s.s == NULL)
    {
        return;
    }

    u64 start = arena_append(&b->add, s.s, s.len);
    u64 new_text_len = s.len;

    if (b->list->len == 0)
    {
        b->list->pieces[0] = (piece){
            .start = start,
            .len = new_text_len,
            .source = ADD
        };
        b->list->len++;
        return;
    }
}

void buffer_build_line_cache(buffer *b)
{
    int i = 0;
    int line_idx = 0;
    line_cache lc = {0};
    lc.line_count = 0;

    if (b->original.len > 0)
    {
        lc.capacity = 256;
        lc.line_starts = malloc(sizeof(int)*lc.capacity);
        if (lc.line_starts == NULL)
        {
            /* @cleanup: logging probaby to file */
            fprintf(stderr, "failed to malloc line cache\n");
            exit(1);
        }

        for (i = 0; i < b->list->len; i++)
        {
            piece p = b->list->pieces[i];

            u8* s;
            if (p.source == ADD)
            {
                s = b->add.data;
            }
            else if (p.source == ORIGINAL)
            {
                s = b->original.s;
            }
            else
            {
                /* @cleanup: logging probaby to file */
                fprintf(stderr, "Unknown source type\n");
                exit(1);
            }


            int pidx;
            for (pidx = p.start; pidx < p.len; pidx++)
            {
                if (s[pidx] == '\n')
                {
                    if (line_idx >= lc.capacity) {
                        int new_capacity = lc.capacity * 2;
                        int *new_line_starts = malloc(sizeof(int)*new_capacity);
                        if (new_line_starts == NULL)
                        {
                            /* @cleanup: logging probaby to file */
                            fprintf(stderr, "failed to resize line cache\n");
                            exit(1);
                        }
                        memcpy(new_line_starts, lc.line_starts, (sizeof(int)*lc.line_count));

                        free(lc.line_starts);
                        lc.line_starts = new_line_starts;
                        lc.capacity = new_capacity;
                    }

                    /* @nocheckin */
                    fprintf(stdout, "%d", (i+1));

                    lc.line_starts[line_idx++] = i+1;
                    lc.line_count++;
                }
            }

        }
    }

    b->lines = lc;

    return;
}

buffer buffer_init(string path, string content)
{
    buffer b = {0};
    b.file_path = path;
    b.original = content;

    b.add = arena_init(MB(1));

    b.list = malloc(sizeof(piece_list));
    if (b.list == NULL)
    {
        fprintf(stderr, "failed to malloc buffer piece_list\n");
        exit(1);
    }

    b.list->capacity = 64;

    b.list->pieces = malloc(sizeof(piece) * b.list->capacity);
    if (b.list->pieces == NULL)
    {
        fprintf(stderr, "failed to malloc pieces\n");
        exit(1);
    }
    b.list->len = 0;
    b.list->pieces[0] = (piece){
        .source = ORIGINAL,
        .start = 0,
        .len = content.len
    };

    buffer_build_line_cache(&b);
    return(b);
}
