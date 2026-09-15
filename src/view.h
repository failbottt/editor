#ifndef VIEW_H
#define VIEW_H

#include "editor.h"
#include "buffer.h"
#include "gfx.h"

typedef struct view
{
    buffer *b;
    u64 cursor_offset;
} view;

void view_draw(view *v);
void view_get_cursor_pos(u64 doc_offset);

#endif
