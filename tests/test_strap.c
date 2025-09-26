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

int main()
{
    test_strtrim();
    test_strtrim_inplace();
    test_strjoin();
    test_strjoin_va();
    test_strstartswith_and_strendswith();
    test_strreplace();
    test_timeval();

    printf("All tests passed!\n");
    return 0;
}
