#ifndef INPUT_H
#define INPUT_H

#include "keydefs.h"
#include "base.h"

typedef struct
{
    u64 value;
} key;

key input_get_key();

#endif
