#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "base.h"
#include "buffer.h"
#include "editor.h"

typedef struct
{
    u8 found;
    u8 at_end;
    u64 index;
    u64 split_at;
} piece_hit;

static u8 *buffer_source(buffer *b, piece *p)
{
    u8 *r = (u8 *)"";
    if (p->source == ADD)
    {
        r = b->add.data;
    }
    else if (p->source == ORIGINAL)
    {
        r = b->original.data;
    }

    return(r);
}

static u64 piece_list_doc_length(piece_list *list)
{
    u64 total = 0;
    u64 i = 0;
    for (i = 0; i < list->len; i++)
    {
       total += list->pieces[i].len;
    }
    return(total);
}

u64 buffer_document_length(buffer *b)
{
    return piece_list_doc_length(b->list);
}

static void ensure_piece_capacity(piece_list *list, u64 needed)
{
    if (needed <= list->capacity)
    {
        return;
    }

    u64 new_capacity = list->capacity ? list->capacity : 1;
    while (new_capacity < needed)
    {
        if (new_capacity > UINT64_MAX / 2)
        {
            fprintf(stderr, "piece list capacity overflow\n");
            exit(1);
        }

        new_capacity *= 2;
    }

    piece *new_pieces = realloc(list->pieces,
            sizeof(piece) * new_capacity);
    if (new_pieces == NULL)
    {
        fprintf(stderr, "failed to grow piece list\n");
        exit(1);
    }

    list->pieces = new_pieces;
    list->capacity = new_capacity;
}

static void insert_piece_at(piece_list *list, u64
        index, piece p)
{
    ensure_piece_capacity(list, list->len + 1);

    if (index < list->len)
    {
        /*
         * shift everything in the array one slot to the right.
         */
        memmove(
                &list->pieces[index + 1],
                &list->pieces[index],
                sizeof(piece) * (list->len - index)
               );
    }

    list->pieces[index] = p;
    list->len++;
}

static u8 pieces_touch(piece a, piece b)
{
    return(a.source == b.source && a.start + a.len == b.start);
}

static void merge_adjacent_pieces(piece_list *list)
{
    u64 read;
    u64 write;

    if (list->len < 2)
    {
        return;
    }

    write = 0;
    for (read = 1; read < list->len; read++)
    {
        if (pieces_touch(list->pieces[write], list->pieces[read]))
        {
            list->pieces[write].len += list->pieces[read].len;
        }
        else
        {
            write++;
            if (write != read)
            {
                list->pieces[write] = list->pieces[read];
            }
        }
    }

    list->len = write + 1;
}

static piece_hit find_piece_at(u64 pos, piece_list *list)
{
    /*
     * EXAMPLE:
     *
     * pos = 23;
     * list = {
     *      {.start = 0, .len = 10}
     *      {.start = 10, .len = 5}
     *      {.start = 15, .len = 10}
     *      {.start = 25, .len = 10}
     *
     * }
     *
     * LOOPS:
     *
     * 1. piece_end = 0 + 10 = 10;
     *    (23 IS NOT LESS THAN 10(piece_end)) SO
     *    total_covered = 10
     *
     * 2. piece_end = 10 + 5 = 15
     *    (23 IS NOT LESS THAN 15(piece_end)) SO
     *    total_covered = 15
     *
     * 3. piece_end = 15 + 10 = 25
     *    (23 IS LESS THAN 25(piece_end)) SO
     *    found piece return it
     */
    u64 total_covered = 0;
    u64 doc_len = piece_list_doc_length(list);

    /* outside the range of the document */
    if (pos > doc_len)
    {
        return((piece_hit){
            .found = FALSE,
            .at_end = FALSE,
            .index = list->len,
            .split_at = 0
        });
    }

    u64 i;
    for (i = 0; i < list->len; i++)
    {
        piece p = list->pieces[i];
        u64 piece_end = total_covered + p.len;

        if (pos < piece_end)
        {
            return((piece_hit){
                .found = TRUE,
                .index = i,
                .split_at = pos - total_covered
            });
        }

        total_covered = piece_end;
    }

    return((piece_hit){
        .found = FALSE,
        .index = list->len,
        .at_end = (pos == doc_len),
        .split_at = 0
    });
}

void buffer_insert(buffer *b, u64 pos, string str)
{
    piece_hit ph;
    u64 start;
    piece new_piece;

    if (b == NULL || b->list == NULL || str.data == NULL || str.len == 0)
    {
        return;
    }

    ph = find_piece_at(pos, b->list);
    if (ph.found == FALSE && ph.at_end == FALSE)
    {
        return;
    }

    start = arena_append(&b->add, str.data, str.len);
    new_piece = (piece){
        .start = start,
        .len = str.len,
        .source = ADD
    };

    if (b->list->len == 0 || ph.at_end)
    {
        insert_piece_at(b->list, b->list->len, new_piece);
    }
    else if (ph.split_at == 0)
    {
        insert_piece_at(b->list, ph.index, new_piece);
    }
    else
    {
        piece p = b->list->pieces[ph.index];
        piece left = {
            .start = p.start,
            .len = ph.split_at,
            .source = p.source
        };
        piece right = {
            .start = p.start + ph.split_at,
            .len = p.len - ph.split_at,
            .source = p.source
        };

        ensure_piece_capacity(b->list, b->list->len + 2);
        memmove(
                &b->list->pieces[ph.index + 3],
                &b->list->pieces[ph.index + 1],
                sizeof(piece) * (b->list->len - ph.index - 1)
                );

        b->list->pieces[ph.index] = left;
        b->list->pieces[ph.index + 1] = new_piece;
        b->list->pieces[ph.index + 2] = right;
        b->list->len += 2;
    }

    merge_adjacent_pieces(b->list);
    buffer_build_line_cache(b);
    b->version++;
    b->dirty = TRUE;
}

void buffer_build_line_cache(buffer *b)
{
    line_cache lc = {0};
    u64 i;
    u64 doc_pos = 0;

    if (b == NULL || b->list == NULL)
    {
        return;
    }

    lc.capacity = 256;
    lc.indexes = malloc(sizeof(u64) * lc.capacity);
    if (lc.indexes == NULL)
    {
        fprintf(stderr, "failed to malloc line cache\n");
        exit(1);
    }

    lc.indexes[0] = 0;
    lc.len = 1;

    for (i = 0; i < b->list->len; i++)
    {
        piece p = b->list->pieces[i];
        u8 *s = buffer_source(b, &p);
        u64 pidx;

        for (pidx = 0; pidx < p.len; pidx++)
        {
            if (s[p.start + pidx] == '\n')
            {
                if (lc.len >= lc.capacity)
                {
                    u64 new_capacity = lc.capacity * 2;
                    u64 *new_line_starts = realloc(
                            lc.indexes,
                            sizeof(u64) * new_capacity
                            );
                    if (new_line_starts == NULL)
                    {
                        fprintf(stderr, "failed to resize line cache\n");
                        exit(1);
                    }

                    lc.indexes = new_line_starts;
                    lc.capacity = new_capacity;
                }

                lc.indexes[lc.len++] = doc_pos + pidx + 1;
            }
        }

        doc_pos += p.len;
    }

    free(b->cached_line_starts.indexes);

    b->cached_line_starts = lc;

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
    b.list->len = 0;

    b.list->pieces = malloc(sizeof(piece) * b.list->capacity);
    if (b.list->pieces == NULL)
    {
        fprintf(stderr, "failed to malloc pieces\n");
        exit(1);
    }
    b.list->pieces[0] = (piece){
        .source = ORIGINAL,
        .start = 0,
        .len = content.len
    };
    b.list->len = (content.len > 0) ? 1 : 0;

    buffer_build_line_cache(&b);
    return(b);
}

void buffer_destroy(buffer *b)
{
    if (b == NULL)
    {
        return;
    }

    free(b->add.data);
    b->add.data = NULL;
    b->add.capacity = 0;
    b->add.pos = 0;
    b->add.prev_pos = 0;

    if (b->list != NULL)
    {
        free(b->list->pieces);
        b->list->pieces = NULL;
        free(b->list);
        b->list = NULL;
    }

    free(b->cached_line_starts.indexes);
    b->cached_line_starts.indexes = NULL;
    b->cached_line_starts.len = 0;
    b->cached_line_starts.capacity = 0;
}

void buffer_delete(buffer *b, u64 start, u64 end)
{
    if (start >= end)
    {
        return;
    }

    piece_list *list;

    if (b == NULL || b->list == NULL)
    {
        return;
    }

    list = b->list;

    u64 doc_len = piece_list_doc_length(list);

    if (start > doc_len || end > doc_len)
    {
        return;
    }

    piece_hit start_ph = find_piece_at(start, list);
    piece_hit end_ph = find_piece_at(end, list);

    if (start_ph.found == FALSE)
    {
        return;
    }

    if (end_ph.found && start_ph.index == end_ph.index)
    {
        piece p = list->pieces[start_ph.index];
        u64 left_len = start_ph.split_at;
        u64 right_len = p.len - end_ph.split_at;

        if (left_len == 0 && right_len == 0)
        {
            memmove(
                    &list->pieces[start_ph.index],
                    &list->pieces[start_ph.index + 1],
                    sizeof(piece) * (list->len - start_ph.index - 1)
                    );
            list->len--;
        }
        else if (left_len == 0)
        {
            list->pieces[start_ph.index].start = p.start + end_ph.split_at;
            list->pieces[start_ph.index].len = right_len;
        }
        else if (right_len == 0)
        {
            list->pieces[start_ph.index].len = left_len;
        }
        else
        {
            ensure_piece_capacity(list, list->len + 1);
            memmove(
                    &list->pieces[start_ph.index + 2],
                    &list->pieces[start_ph.index + 1],
                    sizeof(piece) * (list->len - start_ph.index - 1)
                    );
            list->pieces[start_ph.index] = (piece){
                .start = p.start,
                .len = left_len,
                .source = p.source
            };
            list->pieces[start_ph.index + 1] = (piece){
                .start = p.start + end_ph.split_at,
                .len = right_len,
                .source = p.source
            };
            list->len++;
        }

        merge_adjacent_pieces(list);
        buffer_build_line_cache(b);
        b->version++;
        b->dirty = TRUE;
        return;
    }

    {
        u64 dst = start_ph.index;
        u64 src = list->len;
        u64 tail_count;

        if (start_ph.split_at > 0)
        {
            list->pieces[start_ph.index].len = start_ph.split_at;
            dst = start_ph.index + 1;
        }

        if (end_ph.found)
        {
            if (end_ph.split_at > 0)
            {
                list->pieces[end_ph.index].start += end_ph.split_at;
                list->pieces[end_ph.index].len -= end_ph.split_at;
            }

            src = end_ph.index;
        }

        tail_count = list->len - src;

        memmove(
                &list->pieces[dst],
                &list->pieces[src],
                sizeof(piece) * tail_count
                );

        list->len = dst + tail_count;
    }

    merge_adjacent_pieces(list);
    buffer_build_line_cache(b);
    b->version++;
    b->dirty = TRUE;

    return;
}

cursor_pos buffer_offset_to_screen_pos(buffer *b, u64 offset)
{
    /* the term grid is 1 based not 0 based */
    cursor_pos cursor = {.x = 1, .y = 1};

    u64 x;
    u64 y;

    u64 i;
    for (i = 0; i < b->cached_line_starts.len; i++)
    {
        if (offset < b->cached_line_starts.indexes[i])
        {
            y = i;
            break;
        }
    }

    x = b->cached_line_starts.indexes[cursor.y-1] + offset;

    if (x < 1)
    {
        x++;
    }

    if (y > b->cached_line_starts.indexes[b->cached_line_starts.len-1])
    {
        y = b->cached_line_starts.len;
    }

    cursor.x = x;
    cursor.y = y;

    return(cursor);
}
