#include "../src/editor.h"

editor E;

#include "../src/buffer.c"
#include "../src/funcs.c"
#include "test_funcs.c"

int main()
{
    printf("[starting tests]\n");
    test_funcs_init();
    return 0;
}
