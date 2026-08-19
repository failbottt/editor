#include "../src/buffer.h"

static void
test_buffer_init(buffer *b, const char *text)
{
    string data;
    string path;

    data.s = (u8 *)text;
    data.len = strlen(text);
    path.s = NULL;
    path.len = 0;

    buffer_init(b, data, path);
}

static void
test_buffer_free(buffer *b)
{
    if (b->pieces.items != NULL)
    {
        free(b->pieces.items);
    }

    if (b->add.s != NULL)
    {
        free(b->add.s);
    }

    b->pieces.items = NULL;
    b->pieces.count = 0;
    b->pieces.capacity = 0;
    b->add.s = NULL;
    b->add.len = 0;
    b->add_capacity = 0;
}

