#include "buffer.h"
#include "base.h"

static void
line_index_reserve(line_index *lines, u64 needed_capacity)
{
    u64 new_capacity = lines->capacity ? lines->capacity : 8;
    line_info *new_items;

    while (new_capacity < needed_capacity)
    {
        new_capacity *= 2;
    }

    new_items = (line_info *)realloc(lines->items, sizeof(line_info) * new_capacity);
    if (new_items == NULL)
    {
        fprintf(stderr, "[error] line_index_reserve unable to realloc\n");
        exit(1);
    }

    lines->items = new_items;
    lines->capacity = new_capacity;
}

static void
buffer_rebuild_line_index(buffer *b)
{
    u64 i;
    u64 line_count = 1;

    for (i = 0; i < b->total_len; i++)
    {
        if (buffer_byte_at(b, i) == '\n')
        {
            line_count++;
        }
    }

    if (line_count > b->lines.capacity)
    {
        line_index_reserve(&b->lines, line_count);
    }

    b->lines.count = 0;
    b->lines.items[b->lines.count++] = (line_info){.start = 0};

    for (i = 0; i < b->total_len; i++)
    {
        if (buffer_byte_at(b, i) == '\n')
        {
            b->lines.items[b->lines.count++] = (line_info){.start = i + 1};
        }
    }
}

static piece_loc
find_piece_at_offset(buffer *b, u64 offset)
{
    u64 doc_pos = 0;
    u64 i;

    if (offset > b->total_len)
    {
        fprintf(stderr, "[error] find_piece_at_offset out of bounds\n");
        exit(1);
    }

    for (i = 0; i < b->pieces.count; i++)
    {
        piece p = b->pieces.items[i];

        if (offset < doc_pos + p.len)
        {
            return (piece_loc){
                .piece_index = i,
                .piece_offset = offset - doc_pos,
                .doc_start = doc_pos,
            };
        }

        doc_pos += p.len;
    }

    if (offset == b->total_len && b->pieces.count > 0)
    {
        piece last = b->pieces.items[b->pieces.count - 1];
        return (piece_loc){
            .piece_index = b->pieces.count - 1,
            .piece_offset = last.len,
            .doc_start = b->total_len - last.len,
        };
    }

    fprintf(stderr, "[error] find_piece_at_offset could not locate piece\n");
    exit(1);
}

u8
buffer_byte_at(buffer *b, u64 offset)
{
    string r;

    piece_loc loc = find_piece_at_offset(b, offset);
    piece p = b->pieces.items[loc.piece_index];

    if (p.source == BUFFER_SRC_ORIG)
    {
        r = b->orig;
    }
    else
    {
        r = b->add;
    }

    return r.s[p.start + loc.piece_offset];
}

void
buffer_slice(buffer *b, u64 start, u64 len, string *out)
{
    u64 i;
    u8 *data;

    if (start + len > b->total_len)
    {
        fprintf(stderr, "[error] buffer_slice out of bounds\n");
        exit(1);
    }

    data = (u8 *)malloc(len);
    if (data == NULL)
    {
        perror("[error] unable to alloc buffer slice");
        exit(1);
    }

    for (i = 0; i < len; i++)
    {
        data[i] = buffer_byte_at(b, start + i);
    }

    out->s = data;
    out->len = len;
}



static void
piece_array_reserve(piece_array *a, u64 needed_capacity)
{
    u64 new_capacity = a->capacity ? a->capacity : 8;

    while (new_capacity < needed_capacity)
    {
        new_capacity *= 2;
    }

    piece *new_items = (piece *)realloc(a->items, (sizeof(piece) * new_capacity));
    if (new_items == NULL)
    {
        fprintf(stderr, "[error] piece_array_reserve unable to realloc\n");
        exit(1);
    }

    a->items = new_items;
    a->capacity = new_capacity;
}

static void
buffer_add_append(buffer *b, string text, u64 *start_out)
{
    u8 *new_data;
    u64 needed_capacity;
    u64 new_capacity;

    *start_out = b->add.len;

    if (text.len == 0)
    {
        return;
    }

    needed_capacity = b->add.len + text.len;
    if (needed_capacity > b->add_capacity)
    {
        new_capacity = b->add_capacity ? b->add_capacity : 8;
        while (new_capacity < needed_capacity)
        {
            new_capacity *= 2;
        }

        new_data = (u8 *)realloc(b->add.s, sizeof(u8) * new_capacity);
        if (new_data == NULL)
        {
            fprintf(stderr, "[error] buffer_add_append unable to realloc\n");
            exit(1);
        }

        b->add.s = new_data;
        b->add_capacity = new_capacity;
    }

    memcpy(b->add.s + *start_out, text.s, text.len);
    b->add.len += text.len;
}

static void
piece_array_replace(piece_array *a, u64 index, u64 remove_count, piece *new_pieces, u64 new_count)
{
    u64 i;
    u64 tail_start;
    u64 new_total;
    s64 delta;

    if (index > a->count)
    {
        fprintf(stderr, "[error] piece_array_replace index out of bounds\n");
        exit(1);
    }

    if (index + remove_count > a->count)
    {
        fprintf(stderr, "[error] piece_array_replace range out of bounds\n");
        exit(1);
    }

    new_total = a->count - remove_count + new_count;
    if (new_total > a->capacity)
    {
        piece_array_reserve(a, new_total);
    }

    tail_start = index + remove_count;
    delta = (s64)new_count - (s64)remove_count;

    if (delta > 0)
    {
        for (i = a->count; i > tail_start; i--)
        {
            a->items[i + delta - 1] = a->items[i - 1];
        }
    }
    else if (delta < 0)
    {
        for (i = tail_start; i < a->count; i++)
        {
            a->items[i + delta] = a->items[i];
        }
    }

    for (i = 0; i < new_count; i++)
    {
        a->items[index + i] = new_pieces[i];
    }

    a->count = new_total;
}


static void
piece_array_remove(piece_array *a, u64 index, u64 count)
{
    u64 i;

    if (index > a->count)
    {
        fprintf(stderr, "[error] piece_array_remove index out of bounds\n");
        exit(1);
    }

    if (count == 0)
    {
        return;
    }

    if (index + count > a->count)
    {
        fprintf(stderr, "[error] piece_array_remove range out of bounds\n");
        exit(1);
    }

    for (i = index; i + count < a->count; i++)
    {
        a->items[i] = a->items[i + count];
    }

    a->count -= count;
}


static void
piece_array_insert(piece_array* a, u64 idx, piece p)
{
    if (idx > a->count)
    {
        fprintf(stderr, "[error] piece_array_insert out of bounds\n");
        exit(1);
    }

    if (a->count == a->capacity)
    {
        piece_array_reserve(a, a->count + 1);
    }

    u64 i = 0;
    for (i = a->count; i > idx; i--)
    {
        a->items[i] = a->items[i - 1];
    }

    a->items[idx] = p;
    a->count++;
}

static int
buffer_copy_path_cstr(string path, char **out)
{
    char *c_path;

    if (path.s == NULL || path.len == 0)
    {
        *out = NULL;
        return 0;
    }

    c_path = (char *)malloc((size_t)path.len + 1);
    if (c_path == NULL)
    {
        perror("[error] unable to allocate path copy");
        return -1;
    }

    memcpy(c_path, path.s, (size_t)path.len);
    c_path[path.len] = '\0';
    *out = c_path;
    return 0;
}

void
buffer_init(buffer *b, string data, string path)
{
    char *c_path;
    struct stat st;

    b->file_path = path;
    b->orig = data;
    b->add = (string){0};
    b->add_capacity = 0;
    b->has_file_stat = 0;
    b->lines.items = NULL;
    b->lines.count = 0;
    b->lines.capacity = 0;
    b->total_len = data.len;

    if (buffer_copy_path_cstr(path, &c_path) == 0 && c_path != NULL)
    {
        if (stat(c_path, &st) == 0)
        {
            b->file_stat = st;
            b->has_file_stat = 1;
        }
        free(c_path);
    }

    u64 piece_capacity = 64;
    piece* pieces = (piece*)malloc(sizeof(piece) * piece_capacity);
    if (pieces == NULL)
    {
        fprintf(stderr, "[error] unable to malloc pieces\n");
        exit(1);
    }
    b->pieces.items = pieces;
    b->pieces.capacity = piece_capacity;
    b->pieces.count = 0;

    piece_array_insert(
            &b->pieces,
            0,
            (piece){.source = BUFFER_SRC_ORIG, .start = 0, .len = data.len}
            );

    buffer_rebuild_line_index(b);
}

void
buffer_insert(buffer *b, u64 offset, string text)
{
    piece_loc loc;
    piece old_piece;
    piece add_piece;
    u64 add_start;

    if (offset > b->total_len)
    {
        fprintf(stderr, "[error] buffer_insert out of bounds\n");
        exit(1);
    }

    if (text.len == 0)
    {
        return;
    }

    buffer_add_append(b, text, &add_start);
    add_piece = (piece){.source = BUFFER_SRC_ADD, .start = add_start, .len = text.len};

    if (b->pieces.count == 0)
    {
        piece_array_insert(&b->pieces, 0, add_piece);
        b->total_len += text.len;
        return;
    }

    loc = find_piece_at_offset(b, offset);
    old_piece = b->pieces.items[loc.piece_index];

    if (loc.piece_offset == 0)
    {
        piece_array_insert(&b->pieces, loc.piece_index, add_piece);
    }
    else if (loc.piece_offset == old_piece.len)
    {
        piece_array_insert(&b->pieces, loc.piece_index + 1, add_piece);
    }
    else
    {
        piece replacement[3];

        replacement[0] = (piece){
            .source = old_piece.source,
            .start = old_piece.start,
            .len = loc.piece_offset,
        };

        replacement[1] = add_piece;

        replacement[2] = (piece){
            .source = old_piece.source,
            .start = old_piece.start + loc.piece_offset,
            .len = old_piece.len - loc.piece_offset,
        };

        piece_array_replace(&b->pieces, loc.piece_index, 1, replacement, 3);
    }

    b->total_len += text.len;
    buffer_rebuild_line_index(b);
}

void
buffer_delete(buffer *b, u64 start, u64 len)
{
    u64 i;
    u64 end;
    u64 doc_pos;
    u64 new_count;
    u64 new_capacity;
    piece *new_items;

    if (len == 0)
    {
        return;
    }

    if (start > b->total_len || len > b->total_len - start)
    {
        fprintf(stderr, "[error] buffer_delete out of bounds\n");
        exit(1);
    }

    end = start + len;
    new_capacity = b->pieces.count ? b->pieces.count + 1 : 1;
    new_items = (piece *)malloc(sizeof(piece) * new_capacity);
    if (new_items == NULL)
    {
        fprintf(stderr, "[error] buffer_delete unable to alloc temp pieces\n");
        exit(1);
    }

    doc_pos = 0;
    new_count = 0;
    for (i = 0; i < b->pieces.count; i++)
    {
        piece p = b->pieces.items[i];
        u64 piece_start = doc_pos;
        u64 piece_end = doc_pos + p.len;

        if (piece_end <= start || piece_start >= end)
        {
            new_items[new_count++] = p;
        }
        else
        {
            u64 left_keep = 0;
            u64 right_keep = 0;

            if (start > piece_start)
            {
                left_keep = start - piece_start;
            }

            if (piece_end > end)
            {
                right_keep = piece_end - end;
            }

            if (left_keep > 0)
            {
                new_items[new_count++] = (piece){
                    .source = p.source,
                    .start = p.start,
                    .len = left_keep,
                };
            }

            if (right_keep > 0)
            {
                new_items[new_count++] = (piece){
                    .source = p.source,
                    .start = p.start + (p.len - right_keep),
                    .len = right_keep,
                };
            }
        }

        doc_pos = piece_end;
    }

    if (new_count > b->pieces.capacity)
    {
        piece_array_reserve(&b->pieces, new_count);
    }

    memcpy(b->pieces.items, new_items, sizeof(piece) * new_count);
    b->pieces.count = new_count;
    b->total_len -= len;
    free(new_items);

    buffer_rebuild_line_index(b);
}

u64
buffer_line_start(buffer *b, u64 line)
{
    if (line >= b->lines.count)
    {
        return b->total_len;
    }

    return b->lines.items[line].start;
}

u64
buffer_line_len(buffer *b, u64 line)
{
    u64 start;
    u64 end;

    if (line >= b->lines.count)
    {
        return 0;
    }

    start = b->lines.items[line].start;
    if (line + 1 < b->lines.count)
    {
        end = b->lines.items[line + 1].start;
        if (end > start && buffer_byte_at(b, end - 1) == '\n')
        {
            end--;
        }
    }
    else
    {
        end = b->total_len;
    }

    return end - start;
}

u64
buffer_offset_to_line_col(buffer *b, u64 offset, u64 *line, u64 *col)
{
    u64 i;
    u64 clamped_offset = offset;

    if (clamped_offset > b->total_len)
    {
        clamped_offset = b->total_len;
    }

    *line = 0;
    *col = 0;

    for (i = 0; i + 1 < b->lines.count; i++)
    {
        if (b->lines.items[i + 1].start > clamped_offset)
        {
            break;
        }
        *line = i + 1;
    }

    *col = clamped_offset - b->lines.items[*line].start;
    return clamped_offset;
}

u64
buffer_line_col_to_offset(buffer *b, u64 line, u64 col)
{
    u64 start;
    u64 max_col;

    if (b->lines.count == 0)
    {
        return 0;
    }

    if (line >= b->lines.count)
    {
        return b->total_len;
    }

    start = b->lines.items[line].start;
    max_col = buffer_line_len(b, line);
    if (col > max_col)
    {
        col = max_col;
    }

    return start + col;
}

string buffer_to_string(buffer *b)
{
    u64 len = 0;
    u64 i;
    u64 idx = 0;
    string r = {0};
    u8 *s;

    if (b == NULL)
    {
        return (string){0};
    }

    for (i = 0; i < b->pieces.count; i++)
    {
        len += b->pieces.items[i].len;
    }

    s = (u8 *)malloc(len == 0 ? 1 : (size_t)len);

    ASSERT(s != NULL);

    for (i = 0; i < b->pieces.count; i++)
    {

        piece p = b->pieces.items[i];

        ASSERT(
                p.source == BUFFER_SRC_ORIG ||
                p.source == BUFFER_SRC_ADD
                );

        u8* src;

        if (p.source == BUFFER_SRC_ORIG)
        {
            src = b->orig.s;
        }
        else if (p.source == BUFFER_SRC_ADD)
        {
            src = b->add.s;
        }

        memcpy(s + idx, src + p.start, (size_t)p.len);

        idx += p.len;
    }

    r.s = s;
    r.len = idx;

    return r;
}
