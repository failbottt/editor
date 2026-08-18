#include "view.h"

void
view_set_cursor_from_offset(view *v, buffer *b, u64 offset)
{
    buffer_offset_to_line_col(b, offset, &v->cursor.y, &v->cursor.x);
}
