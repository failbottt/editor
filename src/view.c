#include "view.h"

void view_draw(view *v)
{
    buffer *b = v->b;

    u64 x = 1;
    u64 y = 1;

    u64 i;
    for (i = 0; i < b->list->len; i++)
    {
        piece p = b->list->pieces[i];

        u8 *out;
        if (p.source == ORIGINAL)
        {
            out = b->original.data;
        }
        else if (p.source == ADD)
        {
            out = b->add.data;
        }

        int j;
        for (j = p.start; j < p.start + p.len; j++)
        {
            u8 *c = &out[j];
            if (*c == '\n')
            {
                x = 1;
                y++;
                term_set_cursor_pos(x, y);
                continue;
            }
            term_set_cursor_pos(x, y);
            gfx_draw_text(c, 1);
            x++;
        }
    }
}
