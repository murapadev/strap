#include "../strap.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

void test_strtrim_simd_prototype()
{
    size_t prefix = 128;
    size_t suffix = 200;
    const char *payload = "SIMD hot path!";
    size_t payload_len = strlen(payload);
    size_t total = prefix + payload_len + suffix + 1;

    char *input = malloc(total);
    assert(input);

    memset(input, ' ', prefix);
    memcpy(input + prefix, payload, payload_len);
    memset(input + prefix + payload_len, '\t', suffix);
    input[total - 1] = '\0';

    strap_clear_error();
    char *trimmed = strtrim(input);
    assert(trimmed);
    assert(strcmp(trimmed, payload) == 0);
    assert(strap_last_error() == STRAP_OK);

    free(trimmed);
    free(input);

    strap_clear_error();
    const char utf8_whitespace[] = "\xC2\xA0hello\xC2\xA0"; /* non-breaking spaces */
    char *trimmed_utf8 = strtrim(utf8_whitespace);
    assert(trimmed_utf8 && strcmp(trimmed_utf8, utf8_whitespace) == 0);
    free(trimmed_utf8);

    printf("strtrim SIMD prototype tests passed\n");
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

void test_strjoin_simd_copy()
{
    const size_t part_len = 512;
    char *a = malloc(part_len + 1);
    char *b = malloc(part_len + 1);
    char *c = malloc(part_len + 1);
    assert(a && b && c);

    memset(a, 'A', part_len);
    memset(b, 'B', part_len);
    memset(c, 'C', part_len);
    a[part_len] = '\0';
    b[part_len] = '\0';
    c[part_len] = '\0';

    const char *parts[] = {a, b, c};

    strap_clear_error();
    char *joined = strjoin(parts, 3, "|");
    assert(joined);

    size_t expected_len = part_len * 3 + 2;
    assert(strlen(joined) == expected_len);
    for (size_t i = 0; i < part_len; ++i)
        assert(joined[i] == 'A');
    assert(joined[part_len] == '|');
    for (size_t i = 0; i < part_len; ++i)
        assert(joined[part_len + 1 + i] == 'B');
    assert(joined[part_len * 2 + 1] == '|');
    for (size_t i = 0; i < part_len; ++i)
        assert(joined[part_len * 2 + 2 + i] == 'C');
    assert(strap_last_error() == STRAP_OK);

    free(joined);
    free(a);
    free(b);
    free(c);

    printf("strjoin SIMD prototype tests passed\n");
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

void test_line_buffer()
{
    strap_clear_error();
    strap_line_buffer_init(NULL);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    FILE *tmp = tmpfile();
    assert(tmp);

    fputs("first line\n", tmp);

    char long_line[600];
    for (int i = 0; i < 512; ++i)
        long_line[i] = (char)('A' + (i % 26));
    long_line[512] = '\0';
    fputs(long_line, tmp);
    fputc('\n', tmp);
    fputs("trailing-no-newline", tmp);
    fflush(tmp);
    rewind(tmp);

    strap_line_buffer_t buffer;
    strap_line_buffer_init(&buffer);
    assert(strap_last_error() == STRAP_OK);

    char *line = strap_line_buffer_read(tmp, &buffer);
    assert(line && strcmp(line, "first line") == 0);
    char *buffer_ptr = line;
    assert(strap_last_error() == STRAP_OK);

    line = strap_line_buffer_read(tmp, &buffer);
    assert(line == buffer_ptr);
    assert(strcmp(line, long_line) == 0);
    assert(strap_last_error() == STRAP_OK);

    line = strap_line_buffer_read(tmp, &buffer);
    assert(line == buffer_ptr);
    assert(strcmp(line, "trailing-no-newline") == 0);
    assert(strap_last_error() == STRAP_OK);

    line = strap_line_buffer_read(tmp, &buffer);
    assert(line == NULL);
    assert(strap_last_error() == STRAP_OK);

    strap_line_buffer_free(&buffer);
    fclose(tmp);

    printf("strap_line_buffer tests passed\n");
}

void test_strsplit_limit()
{
    strap_clear_error();
    size_t count = 0;
    char **tokens = strsplit_limit("alpha,beta,gamma", ",", 1, &count);
    assert(tokens && count == 2);
    assert(strcmp(tokens[0], "alpha") == 0);
    assert(strcmp(tokens[1], "beta,gamma") == 0);
    assert(strap_last_error() == STRAP_OK);
    strsplit_free(tokens);

    strap_clear_error();
    tokens = strsplit_limit("a,,b", ",", 0, &count);
    assert(tokens && count == 3);
    assert(strcmp(tokens[0], "a") == 0);
    assert(strcmp(tokens[1], "") == 0);
    assert(strcmp(tokens[2], "b") == 0);
    assert(strap_last_error() == STRAP_OK);
    strsplit_free(tokens);

    strap_clear_error();
    tokens = strsplit_limit("", ",", 0, &count);
    assert(tokens && count == 1);
    assert(strcmp(tokens[0], "") == 0);
    assert(strap_last_error() == STRAP_OK);
    strsplit_free(tokens);

    strap_clear_error();
    tokens = strsplit_limit("anything", "", 0, &count);
    assert(tokens == NULL);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    printf("strsplit_limit tests passed\n");
}

static bool split_whitespace(unsigned char ch, void *userdata)
{
    (void)userdata;
    return ch == ' ' || ch == '\t' || ch == '\n';
}

void test_strsplit_predicate()
{
    strap_clear_error();
    size_t count = 0;
    char **tokens = strsplit_predicate("  foo\tbar baz  ", split_whitespace, NULL, 0, &count);
    assert(tokens && count == 3);
    assert(strcmp(tokens[0], "foo") == 0);
    assert(strcmp(tokens[1], "bar") == 0);
    assert(strcmp(tokens[2], "baz") == 0);
    assert(strap_last_error() == STRAP_OK);
    strsplit_free(tokens);

    strap_clear_error();
    tokens = strsplit_predicate("one two   three four", split_whitespace, NULL, 2, &count);
    assert(tokens && count == 3);
    assert(strcmp(tokens[0], "one") == 0);
    assert(strcmp(tokens[1], "two") == 0);
    assert(strcmp(tokens[2], "three four") == 0);
    assert(strap_last_error() == STRAP_OK);
    strsplit_free(tokens);

    strap_clear_error();
    tokens = strsplit_predicate("", split_whitespace, NULL, 0, &count);
    assert(tokens && count == 0);
    assert(strap_last_error() == STRAP_OK);
    strsplit_free(tokens);

    strap_clear_error();
    tokens = strsplit_predicate("noop", NULL, NULL, 0, &count);
    assert(tokens == NULL);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    printf("strsplit_predicate tests passed\n");
}

void test_strcasecmp_helpers()
{
    strap_clear_error();
    assert(strap_strcasecmp("Hello", "hello") == 0);
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    assert(strap_strcasecmp("abc", "abd") < 0);
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    assert(!strcaseeq("abc", "xyz"));
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    assert(strcaseeq("STRAP", "strap"));
    assert(strap_last_error() == STRAP_OK);

    strap_clear_error();
    bool ok = strcaseeq(NULL, "strap");
    assert(!ok);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    strap_clear_error();
    int cmp = strap_strcasecmp(NULL, "x");
    assert(cmp == 0);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    printf("strap_strcasecmp/strcaseeq tests passed\n");
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

void test_time_local_offset_helpers()
{
    strap_clear_error();
    time_t now = time(NULL);
    int offset_minutes = 0;
    assert(strap_time_local_offset(now, &offset_minutes) == 0);
    assert(offset_minutes >= -14 * 60 && offset_minutes <= 14 * 60);
    assert(strap_last_error() == STRAP_OK);

    struct timeval sample = {now, 123456};
    char buf[64];
    strap_clear_error();
    assert(strap_time_format_iso8601_local(sample, buf, sizeof(buf)) == 0);
    assert(strap_last_error() == STRAP_OK);

    struct timeval parsed;
    int parsed_offset = 0;
    strap_clear_error();
    assert(strap_time_parse_iso8601(buf, &parsed, &parsed_offset) == 0);
    assert(parsed.tv_sec == sample.tv_sec);
    assert(parsed.tv_usec == sample.tv_usec);
    assert(parsed_offset == offset_minutes);

    strap_clear_error();
    assert(strap_time_local_offset(now, NULL) == -1);
    assert(strap_last_error() == STRAP_ERR_INVALID_ARGUMENT);

    printf("time local offset helpers tests passed\n");
}

int main()
{
    test_strtrim();
    test_strtrim_inplace();
    test_strtrim_simd_prototype();
    test_strjoin();
    test_strjoin_simd_copy();
    test_strjoin_va();
    test_strstartswith_and_strendswith();
    test_strreplace();
    test_line_buffer();
    test_strsplit_limit();
    test_strsplit_predicate();
    test_strcasecmp_helpers();
    test_timeval();
    test_locale_helpers();
    test_time_local_offset_helpers();
    test_arena_allocator();
    test_timezone_helpers();

    printf("All tests passed!\n");
    return 0;
}
