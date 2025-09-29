#include "../strap.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_strtrim()
{
    strap_clear_error();
    char *result = strtrim("  hello world  ");
    assert(result && strcmp(result, "hello world") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    strap_clear_error();
    result = strtrim("\t\n  test  \r");
    assert(result && strcmp(result, "test") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    strap_clear_error();
    result = strtrim("");
    assert(result && strcmp(result, "") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    strap_clear_error();
    result = strtrim(NULL);
    assert(result == NULL);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    printf("strtrim tests passed\n");
}

void test_strtrim_inplace()
{
    strap_clear_error();
    char buf[] = "  hello  ";
    strtrim_inplace(buf);
    assert(strcmp(buf, "hello") == 0);
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    char buf2[] = "\t test \n";
    strtrim_inplace(buf2);
    assert(strcmp(buf2, "test") == 0);
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    strtrim_inplace(NULL);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    printf("strtrim_inplace tests passed\n");
}

void test_strjoin()
{
    strap_clear_error();
    const char *parts[] = {"hello", "world", "test"};
    char *result = strjoin(parts, 3, " ");
    assert(result && strcmp(result, "hello world test") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    strap_clear_error();
    result = strjoin(parts, 1, ",");
    assert(result && strcmp(result, "hello") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    printf("strjoin tests passed\n");
}

void test_strjoin_va()
{
    strap_clear_error();
    char *result = strjoin_va(" ", "hello", "world", NULL);
    assert(result && strcmp(result, "hello world") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    strap_clear_error();
    result = strjoin_va("-", "a", "b", "c", NULL);
    assert(result && strcmp(result, "a-b-c") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    printf("strjoin_va tests passed\n");
}

void test_strstartswith_and_strendswith()
{
    strap_clear_error();
    assert(strstartswith("strap", "str"));
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    assert(!strstartswith("strap", "zap"));
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    assert(strendswith("strap", "ap"));
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    assert(!strendswith("strap", "strapper"));
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    bool ok = strstartswith(NULL, "foo");
    assert(!ok);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    strap_clear_error();
    ok = strendswith("foo", NULL);
    assert(!ok);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    printf("strstartswith/strendswith tests passed\n");
}

void test_strreplace()
{
    strap_clear_error();
    char *result = strreplace("foo bar foo", "foo", "baz");
    assert(result && strcmp(result, "baz bar baz") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    strap_clear_error();
    result = strreplace("hello", "world", "strap");
    assert(result && strcmp(result, "hello") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    strap_clear_error();
    result = strreplace("aaaa", "aa", "a");
    assert(result && strcmp(result, "aa") == 0);
    assert(strap_last_error() == STRAP_OK);
    free(result);

    strap_clear_error();
    result = strreplace(NULL, "foo", "bar");
    assert(result == NULL);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    strap_clear_error();
    result = strreplace("foo", "", "bar");
    assert(result == NULL);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    printf("strreplace tests passed\n");
}

void test_timeval()
{
    struct timeval a = {1, 500000};
    struct timeval b = {2, 600000};
    struct timeval sum = timeval_add(a, b);
    assert(sum.tv_sec == 4 && sum.tv_usec == 100000);

    struct timeval diff = timeval_sub(b, a);
    assert(diff.tv_sec == 1 && diff.tv_usec == 100000);

    double sec = timeval_to_seconds(a);
    assert(sec == 1.5);

    printf("timeval tests passed\n");
}

void test_locale_helpers()
{
    strap_clear_error();
    char *lower = strtolower_locale("HELLO", "C");
    assert(lower && strcmp(lower, "hello") == 0);
    free(lower);

    strap_clear_error();
    char *upper = strtoupper_locale("strap", NULL);
    assert(upper && strcmp(upper, "STRAP") == 0);
    free(upper);

    strap_clear_error();
    int cmp = strcasecmp_locale("StraP", "strap", "C");
    assert(cmp == 0);
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    cmp = strcoll_locale("abc", "abd", NULL);
    assert(cmp < 0);

    strap_clear_error();
    char *invalid = strtolower_locale(NULL, NULL);
    assert(!invalid);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    printf("locale helper tests passed\n");
}

void test_arena_allocator()
{
    strap_arena_t *arena = strap_arena_create(0);
    assert(arena);

    const char *parts[] = {"a", "b", "c"};
    char *joined = strjoin_arena(arena, parts, 3, "-");
    assert(joined && strcmp(joined, "a-b-c") == 0);

    char *trimmed = strtrim_arena(arena, "  hello  ");
    assert(trimmed && strcmp(trimmed, "hello") == 0);

    char *replaced = strreplace_arena(arena, "foofoo", "foo", "bar");
    assert(replaced && strcmp(replaced, "barbar") == 0);

    char *upper = strtoupper_locale_arena(arena, "abc", "C");
    assert(upper && strcmp(upper, "ABC") == 0);

    void *mem = strap_arena_alloc(arena, 16);
    assert(mem);

    strap_arena_clear(arena);
    mem = strap_arena_alloc(arena, 8);
    assert(mem);

    strap_arena_destroy(arena);
    printf("arena allocator tests passed\n");
}

void test_timezone_helpers()
{
    strap_clear_error();
    char buffer[64];
    assert(strap_time_offset_to_string(0, buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "Z") == 0);

    strap_clear_error();
    assert(strap_time_offset_to_string(330, buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "+05:30") == 0);

    strap_clear_error();
    int offset = 0;
    assert(strap_time_parse_tz_offset("-03:00", &offset) == 0);
    assert(offset == -180);

    strap_clear_error();
    assert(strap_time_parse_tz_offset("+01", &offset) == 0);
    assert(offset == 60);

    strap_clear_error();
    struct timeval tv = {0, 0};
    assert(strap_time_format_iso8601(tv, 0, buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "1970-01-01T00:00:00Z") == 0);

    strap_clear_error();
    struct timeval tv_micro = {0, 123456};
    assert(strap_time_format_iso8601(tv_micro, 60, buffer, sizeof(buffer)) == 0);
    assert(strcmp(buffer, "1970-01-01T01:00:00.123456+01:00") == 0);

    strap_clear_error();
    struct timeval parsed;
    int parsed_offset = 0;
    assert(strap_time_parse_iso8601("1970-01-01T01:00:00.123456+01:00", &parsed, &parsed_offset) == 0);
    assert(parsed.tv_sec == 0 && parsed.tv_usec == 123456);
    assert(parsed_offset == 60);

    printf("timezone helper tests passed\n");
}

int main()
{
    test_strtrim();
    test_strtrim_inplace();
    test_strjoin();
    test_strjoin_va();
    test_strstartswith_and_strendswith();
    test_strreplace();
    test_timeval();
    test_locale_helpers();
    test_arena_allocator();
    test_timezone_helpers();

    printf("All tests passed!\n");
    return 0;
}
