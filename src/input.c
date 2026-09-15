#include <unistd.h>

#include "input.h"

key input_get_key()
{
    char b[32];
    int n = read(STDIN_FILENO, &b, sizeof(b));
    if (n <= -1)
    {
        return (key){0};
    }
    key r = {.value = *b};
    return(r);
}


