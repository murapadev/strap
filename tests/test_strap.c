#include "../strap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_strtrim()
{
    char *result = strtrim("  hello world  ");
    assert(result && strcmp(result, "hello world") == 0);
    free(result);

    result = strtrim("\t\n  test  \r");
    assert(result && strcmp(result, "test") == 0);
    free(result);

    result = strtrim("");
    assert(result && strcmp(result, "") == 0);
    free(result);

    printf("strtrim tests passed\n");
}

void test_strtrim_inplace()
{
    char buf[] = "  hello  ";
    strtrim_inplace(buf);
    assert(strcmp(buf, "hello") == 0);

    char buf2[] = "\t test \n";
    strtrim_inplace(buf2);
    assert(strcmp(buf2, "test") == 0);

    printf("strtrim_inplace tests passed\n");
}

void test_strjoin()
{
    const char *parts[] = {"hello", "world", "test"};
    char *result = strjoin(parts, 3, " ");
    assert(result && strcmp(result, "hello world test") == 0);
    free(result);

    result = strjoin(parts, 1, ",");
    assert(result && strcmp(result, "hello") == 0);
    free(result);

    printf("strjoin tests passed\n");
}

void test_strjoin_va()
{
    char *result = strjoin_va(" ", "hello", "world", NULL);
    assert(result && strcmp(result, "hello world") == 0);
    free(result);

    result = strjoin_va("-", "a", "b", "c", NULL);
    assert(result && strcmp(result, "a-b-c") == 0);
    free(result);

    printf("strjoin_va tests passed\n");
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
    test_timeval();

    printf("All tests passed!\n");
    return 0;
}