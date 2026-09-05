#include "view.h"

void view_draw(buffer *b)
{
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

        u64 j;
        for (j = p.start; j < p.start + p.len; j++)
        {
            gfx_draw_text(out+j, 1);
        }
    }
}
