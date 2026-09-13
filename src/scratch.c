#include <unistd.h>
#include <string.h>

#include "base.h"
#include "buffer.h"

int main()
{

    buffer b = buffer_init(STR("/path/to/file"), STR("foo bar"));

    buffer_insert(&b, 4, STR("hello world "));

    buffer_insert(&b, 16, STR("guy"));

    buffer_insert(&b, 19, STR("baz<SPACE>"));

    buffer_delete(&b, 10, 29);

    piece_list *list = b.list;
    int i;
    for (i = 0; i < list->len; i++)
    {
        piece p = list->pieces[i];

        u8 *src = NULL;
        if (p.source == ADD)
        {
            src = b.add.data;
        }
        else if (p.source == ORIGINAL)
        {
            src = b.original.data;
        }

        write(STDOUT_FILENO, (const char*)src+p.start, sizeof(u8) * p.len);
    }

    fprintf(stdout, "\n");

    return 0;
}
