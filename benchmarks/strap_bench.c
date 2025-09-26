#include "../strap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

static double elapsed_seconds(struct timeval start, struct timeval end)
{
    struct timeval diff = timeval_sub(end, start);
    return timeval_to_seconds(diff);
}

static void *xmalloc(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
    {
        fprintf(stderr, "Allocation failed (%zu bytes)\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static void bench_strjoin(size_t iterations)
{
    const size_t parts_count = 8;
    const size_t part_len = 64;

    char **owned_parts = xmalloc(parts_count * sizeof(char *));
    const char **parts = xmalloc(parts_count * sizeof(char *));
    for (size_t i = 0; i < parts_count; ++i)
    {
        owned_parts[i] = xmalloc(part_len + 1);
        memset(owned_parts[i], 'A' + (int)i, part_len);
        owned_parts[i][part_len] = '\0';
        parts[i] = owned_parts[i];
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (size_t i = 0; i < iterations; ++i)
    {
        char *joined = strjoin(parts, parts_count, ",");
        if (!joined)
        {
            fprintf(stderr, "strjoin failed: %s\n", strap_error_string(strap_last_error()));
            exit(EXIT_FAILURE);
        }
        free(joined);
    }
    gettimeofday(&end, NULL);

    double secs = elapsed_seconds(start, end);
    printf("strjoin (%zu iterations): %.3f ms\n", iterations, secs * 1000.0);

    for (size_t i = 0; i < parts_count; ++i)
        free(owned_parts[i]);
    free(owned_parts);
    free(parts);
}

static void bench_strtrim(size_t iterations)
{
    const char *input = "\t    strap trims strings nicely    \n";

    struct timeval start, end;

    gettimeofday(&start, NULL);
    for (size_t i = 0; i < iterations; ++i)
    {
        char *trimmed = strtrim(input);
        if (!trimmed)
        {
            fprintf(stderr, "strtrim failed: %s\n", strap_error_string(strap_last_error()));
            exit(EXIT_FAILURE);
        }
        free(trimmed);
    }
    gettimeofday(&end, NULL);
    double secs = elapsed_seconds(start, end);
    printf("strtrim (heap, %zu iterations): %.3f ms\n", iterations, secs * 1000.0);

    gettimeofday(&start, NULL);
    for (size_t i = 0; i < iterations; ++i)
    {
        char buffer[128];
        strcpy(buffer, input);
        strtrim_inplace(buffer);
    }
    gettimeofday(&end, NULL);
    secs = elapsed_seconds(start, end);
    printf("strtrim_inplace (%zu iterations): %.3f ms\n", iterations, secs * 1000.0);
}

static void bench_strreplace(size_t iterations)
{
    const char *sample = "strap allows strap developers to replace strap tokens";

    struct timeval start, end;
    gettimeofday(&start, NULL);
    for (size_t i = 0; i < iterations; ++i)
    {
        char *replaced = strreplace(sample, "strap", "STRAP");
        if (!replaced)
        {
            fprintf(stderr, "strreplace failed: %s\n", strap_error_string(strap_last_error()));
            exit(EXIT_FAILURE);
        }
        free(replaced);
    }
    gettimeofday(&end, NULL);

    double secs = elapsed_seconds(start, end);
    printf("strreplace (%zu iterations): %.3f ms\n", iterations, secs * 1000.0);
}

int main(int argc, char **argv)
{
    size_t iterations = 50000;
    if (argc > 1)
    {
        iterations = (size_t)strtoul(argv[1], NULL, 10);
        if (iterations == 0)
            iterations = 1;
    }

    printf("STRAP micro-benchmarks (iterations=%zu)\n", iterations);
    bench_strjoin(iterations);
    bench_strtrim(iterations);
    bench_strreplace(iterations);

    return 0;
}
