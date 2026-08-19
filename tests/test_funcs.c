#include <stdio.h>
#include <string.h>

#include "common.h"
#include "../src/funcs.h"
#include "../src/buffer.h"
#include "../src/base.h"

static void
test_join_lines()
{
    buffer b = {0};
    string out;
    const char *expected = "foo bar\nbaz";

    test_buffer_init(&b, "foo\nbar\nbaz");

    join_lines(&b, 0);

    out = buffer_to_string(&b);
    ASSERT(out.len == strlen(expected));
    ASSERT(memcmp(out.s, expected, out.len) == 0);
    ASSERT(b.total_len == strlen(expected));
    ASSERT(b.lines.count == 2);
    ASSERT(buffer_line_start(&b, 0) == 0);
    ASSERT(buffer_line_start(&b, 1) == 8);
    ASSERT(buffer_line_len(&b, 0) == 7);
    ASSERT(buffer_line_len(&b, 1) == 3);

    free(out.s);
    test_buffer_free(&b);
    printf("%s... OK\n", "test_join_lines");
}

static void
test_insert_at_end_of_line()
{
    buffer b = {0};
    string A = {.s = (u8*)"A", .len = 1};
    string out;
    const char *expected = "foobarA";

    test_buffer_init(&b, "foobar");

    u64 insert_off = insert_at_end_of_line(&b, 0);
    buffer_insert(&b, insert_off, A);

    out = buffer_to_string(&b);
    ASSERT(out.len == strlen(expected));
    ASSERT(memcmp(out.s, expected, out.len) == 0);
    ASSERT(b.total_len == strlen(expected));

    printf("%s... OK\n", "test_insert_at_end_of_line");
}

static void
test_insert_above_current_line()
{
    buffer b = {0};
    string A = {.s = (u8*)"A", .len = 1};
    string out;
    const char *expected = "A\nfoobar";

    test_buffer_init(&b, "foobar");

    u64 insert_off = insert_above_current_line(&b, 0);
    buffer_insert(&b, insert_off, A);

    out = buffer_to_string(&b);
    ASSERT(out.len == strlen(expected));
    ASSERT(memcmp(out.s, expected, out.len) == 0);
    ASSERT(b.total_len == strlen(expected));

    printf("%s... OK\n", "test_insert_above_current_line");
}

static void
test_insert_below_current_line()
{
    buffer b = {0};
    string A = {.s = (u8*)"A", .len = 1};
    string out;
    const char *expected = "foobar\nA";

    test_buffer_init(&b, "foobar");

    u64 insert_off = insert_below_current_line(&b, 0);

    buffer_insert(&b, insert_off + 1, A);

    out = buffer_to_string(&b);

    ASSERT(out.len == strlen(expected));
    ASSERT(memcmp(out.s, expected, out.len) == 0);
    ASSERT(b.total_len == strlen(expected));

    printf("%s... OK\n", "test_insert_below_current_line");
}

static void
test_funcs_init()
{
    test_join_lines();
    test_insert_at_end_of_line();
    test_insert_above_current_line();
    test_insert_below_current_line();
}
