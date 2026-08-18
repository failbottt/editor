#ifndef VIEW_H
#define VIEW_H

#include "base.h"
#include "cursor.h"
#include "buffer.h"

typedef struct {
    u32 buffer_id;
    cursor cursor;
    int rowoff;     /* Offset of row displayed. */
    int coloff;     /* Offset of column displayed. */
} view;

void view_set_cursor_from_offset(view *v, buffer *b, u64 offset);

#endif
