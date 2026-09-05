#ifndef BASE_H
#define BASE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define KB(x) 1 << 10
#define MB(x) 1 << 20
#define GB(x) 1 << 30
#define TB(x) 1 << 40

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define s8  int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t

#define TRUE 1
#define FALSE 0

typedef struct
{
   u8 *data;
   u64 len;
} string;

typedef struct
{
    u8 *data;
    u64 capacity;
    u64 pos;
    u64 prev_pos;
} arena;

static arena arena_init(int size)
{
    arena r = {0};
    r.pos = 0;
    r.prev_pos = 0;
    r.capacity = 0;

    if (size <= 0)
    {
        return(r);
    }

    r.data = (u8*)malloc(sizeof(u8)*size);
    if (r.data == NULL)
    {
        fprintf(stderr, "[arena_init]: Unable to malloc new arena");
        exit(1);
    }

    r.capacity = size;

    return(r);
}

/* @todo: make this more general where any data structure
 * can be added here. make sure to add alignment handling */
static u64 arena_append(arena *a, u8 *data, u64 len)
{
    if (a->data == NULL || len == 0)
    {
        return 0;
    }

    if (a->pos + len > a->capacity)
    {
        u64 needed = a->pos + len;
        if (a->pos > UINT64_MAX - len)
        {
            fprintf(stderr, "[arena_append] UINT64_MAX overflow");
            exit(1);
        }

        u64 newcap = a->capacity ? a->capacity : 1;
        while (newcap < needed)
        {
            if (newcap > UINT64_MAX / 2)
            {
                fprintf(stderr, "[arena_append] UINT64_MAX overflow for new capacity");
                exit(1);
            }
            newcap *= 2;
        }

        u8 *new_buffer = (u8*)malloc(sizeof(u8)*newcap);
        if (new_buffer == NULL)
        {
            fprintf(stderr, "[arena_append]: Unable to malloc buffer.");
            exit(1);
        }

        memcpy(new_buffer, a->data, sizeof(u8)*a->pos);

        free(a->data);

        a->data = new_buffer;
        a->capacity = newcap;
    }

    memcpy(a->data+a->pos, data, sizeof(u8)*len);
    a->prev_pos = a->pos;
    a->pos = a->pos + len;
    return a->prev_pos;
}

#endif
