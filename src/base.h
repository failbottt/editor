#ifndef BASE_H
#define BASE_H

#include <stdint.h>

/* types */
#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define s8  int8_t
#define s16 int16_t
#define s32 int32_t
#define s64 int64_t
#define f32 float
#define f64 double

typedef struct
{
    u64 len;
    u8* s;
} string;

/* defines */
#define TRUE 1;
#define FALSE 0;

/* macros */
#define KB(x) (x << 10)
#define MB(x) (x << 20)
#define GB(x) (x << 30)

/* file */

/* @cleanup: clanked */
#include <error.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

static int
readfile(const char *path, string *out, size_t max_size)
{
    int fd = -1;
    struct stat st;
    char *buf = NULL;
    size_t off = 0;
    size_t len;

    out->s = NULL;
    out->len = 0;

    fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }

    if (fstat(fd, &st) != 0)
    {
        goto fail;
    }

    if (!S_ISREG(st.st_mode) || st.st_size < 0)
    {
        errno = EINVAL;
        goto fail;
    }

    if ((unsigned long long)st.st_size > (unsigned long long)max_size)
    {
        errno = EFBIG;
        goto fail;
    }

    len = (size_t)st.st_size;
    buf = (char *)malloc(len + 1);
    if (!buf)
    {
        goto fail;
    }

    while (off < len)
    {
        ssize_t n = read(fd, buf + off, len - off);
        if (n > 0)
        {
            off += (size_t)n;
            continue;
        }
        if (n == 0)
        {
            errno = EIO;
            goto fail;
        }
        if (errno == EINTR)
        {
            continue;
        }
        goto fail;
    }

    buf[len] = '\0';
    out->s = (u8 *)buf;
    out->len = len;

    if (close(fd) != 0)
    {
        free(buf);
        out->s = NULL;
        out->len = 0;
        return -1;
    }

    return 0;

fail:
    {
        int saved = errno;
        if (fd >= 0)
        {
            close(fd);
        }
        free(buf);
        errno = saved;
        return -1;
    }
}

/* arena */
#include <stdlib.h>
#include <string.h>

typedef struct
{
    u64 cap;
    u64 cur_pos;
    u8* data;
} arena;

static arena
new_arena(u64 size)
{

    u8* data = (u8*)malloc(sizeof(u8)*size);
    if (data == NULL)
    {
        perror("[error] unable to malloc for new arena");
        exit(1);
    }

    arena r = {0};
    r.cap = size;
    r.cur_pos = 0;
    r.data = data;

    return r;
}

static void
arena_push_array(arena *arena, u8* data, u64 len)
{
    if (len + arena->cur_pos >= arena->cap)
    {
        u64 new_cap = arena->cap * 2;
        u8 *new_buffer = (u8*)malloc((sizeof(u8) * new_cap));
        if (new_buffer == NULL)
        {
            perror("[error] unable to increase arena size");
            exit(1);
        }

        memcpy(new_buffer, arena->data, arena->cur_pos);
        arena->data = new_buffer;
        arena->cap = new_cap;
        free(arena->data);
    }

    memcpy(arena->data + arena->cur_pos, data, len);
    arena->cur_pos += len;
}


#endif
