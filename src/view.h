#ifndef VIEW_H
#define VIEW_H

#include "base.h"
#include "cursor.h"

typedef struct {
    u32 buffer_id;
    cursor cursor;
    int rowoff;     /* Offset of row displayed. */
    int coloff;     /* Offset of column displayed. */
} view;

#endif
