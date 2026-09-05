#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "base.h"
#include "buffer.h"

typedef struct
{
    u8 found;
    u8 at_end;
    u64 index;
    u64 split_at;
} piece_hit;

static u64 piece_list_doc_length(piece_list *list)
{
    u64 total = 0;
    int i = 0;
    for (i = 0; i < list->len; i++)
    {
       total += list->pieces[i].len;
    }
    return(total);
}

static void ensure_piece_capacity(piece_list *list)
{
    if (list->len < list->capacity)
    {
        return;
    }

    u64 new_capacity = list->capacity ? list->capacity
        * 2 : 1;
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
    ensure_piece_capacity(list);

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
  if (str.data == NULL || str.data == 0)
  {
      return;
  }

  u64 start = arena_append(&b->add, str.data, str.len);

  if (b->list->len == 0)
  {
      b->list->pieces[0] = (piece){
          .start = start,
          .len = str.len,
          .source = ADD
      };
      b->list->len = 1;
      return;
  }

  piece_hit ph = find_piece_at(pos, b->list);

  if (ph.found == FALSE)
  {
      if (ph.at_end == FALSE)
      {
          fprintf(stderr, "insert position is outside the document\n");
          return;
      }

      insert_piece_at(b->list, b->list->len, (piece){
          .start = start,
          .len = str.len,
          .source = ADD
      });
      return;
  }

  piece p = b->list->pieces[ph.index];

  if (ph.split_at == 0)
  {
      insert_piece_at(b->list, ph.index, (piece){
          .start = start,
          .len = str.len,
          .source = ADD
      });
      return;
  }

  if (ph.split_at == p.len)
  {
      insert_piece_at(b->list, ph.index + 1, (piece){
          .start = start,
          .len = str.len,
          .source = ADD
      });
      return;
  }

  piece left = {
      .start = p.start,
      .len = ph.split_at,
      .source = p.source
  };

  piece middle = {
      .start = start,
      .len = str.len,
      .source = ADD
  };

  piece right = {
      .start = p.start + ph.split_at,
      .len = p.len - ph.split_at,
      .source = p.source
  };

  ensure_piece_capacity(b->list);
  memmove(
      &b->list->pieces[ph.index + 3], /* dest */
      &b->list->pieces[ph.index + 1], /* src */
      (sizeof(piece) * (b->list->len - ph.index - 1)) /* N bytes to move */
  );

  b->list->pieces[ph.index] = left;
  b->list->pieces[ph.index + 1] = middle;
  b->list->pieces[ph.index + 2] = right;
  b->list->len += 2;
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
                s = b->original.data;
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
    b.list->pieces[0] = (piece){
        .source = ORIGINAL,
        .start = 0,
        .len = content.len
    };
    b.list->len = (content.len > 0) ? 1 : 0;

    buffer_build_line_cache(&b);
    return(b);
}
