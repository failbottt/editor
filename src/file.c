#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include "file.h"

static int
buffer_copy_path_cstr(string path, char **out)
{
    char *c_path;

    if (path.s == NULL || path.len == 0)
    {
        *out = NULL;
        return 0;
    }

    c_path = (char *)malloc((size_t)path.len + 1);
    if (c_path == NULL)
    {
        perror("[error] unable to allocate path copy");
        return -1;
    }

    memcpy(c_path, path.s, (size_t)path.len);
    c_path[path.len] = '\0';
    *out = c_path;
    return 0;
}

static int
buffer_write_all(int fd, u8 *data, u64 len)
{
    u64 off = 0;

    while (off < len)
    {
        ssize_t n = write(fd, data + off, (size_t)(len - off));
        if (n > 0)
        {
            off += (u64)n;
            continue;
        }

        if (n < 0 && errno == EINTR)
        {
            continue;
        }

        return -1;
    }

    return 0;
}

static int
buffer_paths_match_stat(buffer *b, struct stat *current)
{
    if (!b->has_file_stat)
    {
        return 1;
    }

    if (b->file_stat.st_dev != current->st_dev)
    {
        return 0;
    }

    if (b->file_stat.st_ino != current->st_ino)
    {
        return 0;
    }

    if (b->file_stat.st_size != current->st_size)
    {
        return 0;
    }

    if (b->file_stat.st_mtime != current->st_mtime)
    {
        return 0;
    }

    return 1;
}

write_file_result
write_file(buffer *b, int force)
{
    char *path_c = NULL;
    char *tmp_c = NULL;
    string contents;
    struct stat current;
    struct stat written;
    int fd = -1;
    size_t tmp_len;
    write_file_result result = {0};

    result.status = WRITE_FILE_OK;
    result.path.s = NULL;
    result.path.len = 0;
    if (b != NULL)
    {
        result.path = b->file_path;
    }

    if (b == NULL || b->file_path.s == NULL || b->file_path.len == 0)
    {
        result.status = WRITE_FILE_NO_PATH;
        return result;
    }

    if (buffer_copy_path_cstr(b->file_path, &path_c) != 0)
    {
        result.status = WRITE_FILE_OPEN_FAILED;
        return result;
    }

    if (!force && b->has_file_stat)
    {
        if (stat(path_c, &current) != 0)
        {
            free(path_c);
            result.status = WRITE_FILE_NEEDS_CONFIRMATION;
            return result;
        }

        if (!buffer_paths_match_stat(b, &current))
        {
            free(path_c);
            result.status = WRITE_FILE_NEEDS_CONFIRMATION;
            return result;
        }
    }

    contents = buffer_to_string(b);
    result.bytes_written = contents.len;
    result.line_count = b->lines.count;

    tmp_len = (size_t)b->file_path.len + 7;
    tmp_c = (char *)malloc(tmp_len + 1);
    if (tmp_c == NULL)
    {
        free(contents.s);
        free(path_c);
        result.status = WRITE_FILE_OPEN_FAILED;
        return result;
    }

    memcpy(tmp_c, path_c, (size_t)b->file_path.len);
    memcpy(tmp_c + (size_t)b->file_path.len, ".XXXXXX", 7);
    tmp_c[tmp_len] = '\0';

    fd = mkstemp(tmp_c);
    if (fd < 0)
    {
        result.status = WRITE_FILE_OPEN_FAILED;
        goto cleanup;
    }

    if (b->has_file_stat)
    {
        (void)fchmod(fd, b->file_stat.st_mode & 0777);
    }

    if (buffer_write_all(fd, contents.s, contents.len) != 0)
    {
        result.status = WRITE_FILE_WRITE_FAILED;
        goto cleanup;
    }

    if (fsync(fd) != 0)
    {
        result.status = WRITE_FILE_WRITE_FAILED;
        goto cleanup;
    }

    if (close(fd) != 0)
    {
        fd = -1;
        result.status = WRITE_FILE_WRITE_FAILED;
        goto cleanup;
    }

    fd = -1;

    if (rename(tmp_c, path_c) != 0)
    {
        result.status = WRITE_FILE_RENAME_FAILED;
        goto cleanup;
    }

    if (stat(path_c, &written) != 0)
    {
        result.status = WRITE_FILE_STAT_FAILED;
        goto cleanup;
    }

    b->file_stat = written;
    b->has_file_stat = 1;
    result.written_stat = written;
    result.status = WRITE_FILE_OK;

cleanup:
    if (fd >= 0)
    {
        close(fd);
    }

    if (tmp_c != NULL)
    {
        unlink(tmp_c);
        free(tmp_c);
    }

    free(contents.s);
    free(path_c);
    return result;
}
