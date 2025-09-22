#include "strap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

/* Safe reading */
char *afgets(FILE *f)
{
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    if ((read = getline(&line, &len, f)) != -1)
    {
        // Remove newline if present
        if (read > 0 && line[read - 1] == '\n')
        {
            line[read - 1] = '\0';
        }
        return line;
    }
    free(line);
    return NULL;
}

char *afread(FILE *f, size_t *out_len)
{
    if (fseek(f, 0, SEEK_END) != 0)
        return NULL;
    long size = ftell(f);
    if (size < 0)
        return NULL;
    if (fseek(f, 0, SEEK_SET) != 0)
        return NULL;

    char *buffer = malloc(size + 1);
    if (!buffer)
        return NULL;

    size_t read = fread(buffer, 1, size, f);
    if (read != (size_t)size)
    {
        free(buffer);
        return NULL;
    }
    buffer[size] = '\0';
    if (out_len)
        *out_len = size;
    return buffer;
}

/* String manipulation */
char *strjoin(const char **parts, size_t nparts, const char *sep)
{
    if (!parts || nparts == 0)
        return strdup("");

    size_t sep_len = sep ? strlen(sep) : 0;
    size_t total_len = 0;

    for (size_t i = 0; i < nparts; ++i)
    {
        if (parts[i])
            total_len += strlen(parts[i]);
    }
    total_len += (nparts - 1) * sep_len + 1; // +1 for null terminator

    char *result = malloc(total_len);
    if (!result)
        return NULL;

    result[0] = '\0';
    for (size_t i = 0; i < nparts; ++i)
    {
        if (i > 0 && sep)
            strcat(result, sep);
        if (parts[i])
            strcat(result, parts[i]);
    }

    return result;
}

char *strjoin_va(const char *sep, ...)
{
    va_list args;
    va_start(args, sep);

    size_t count = 0;
    const char *part;
    while ((part = va_arg(args, const char *)) != NULL)
    {
        ++count;
    }
    va_end(args);

    if (count == 0)
        return strdup("");

    const char **parts = malloc(count * sizeof(char *));
    if (!parts)
        return NULL;

    va_start(args, sep);
    for (size_t i = 0; i < count; ++i)
    {
        parts[i] = va_arg(args, const char *);
    }
    va_end(args);

    char *result = strjoin(parts, count, sep);
    free(parts);
    return result;
}

/* Trim */
char *strtrim(const char *s)
{
    if (!s)
        return NULL;

    const char *start = s;
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r'))
        ++start;

    const char *end = start + strlen(start) - 1;
    while (end >= start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
        --end;

    size_t len = end - start + 1;
    char *result = malloc(len + 1);
    if (!result)
        return NULL;

    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

void strtrim_inplace(char *s)
{
    if (!s)
        return;

    char *start = s;
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r'))
        ++start;

    if (start != s)
    {
        memmove(s, start, strlen(start) + 1);
    }

    char *end = s + strlen(s) - 1;
    while (end >= s && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r'))
    {
        *end = '\0';
        --end;
    }
}

/* Time utilities */
struct timeval timeval_add(struct timeval a, struct timeval b)
{
    struct timeval result;
    result.tv_sec = a.tv_sec + b.tv_sec;
    result.tv_usec = a.tv_usec + b.tv_usec;
    if (result.tv_usec >= 1000000)
    {
        result.tv_sec += 1;
        result.tv_usec -= 1000000;
    }
    return result;
}

struct timeval timeval_sub(struct timeval a, struct timeval b)
{
    struct timeval result;
    result.tv_sec = a.tv_sec - b.tv_sec;
    result.tv_usec = a.tv_usec - b.tv_usec;
    if (result.tv_usec < 0)
    {
        result.tv_sec -= 1;
        result.tv_usec += 1000000;
    }
    return result;
}

double timeval_to_seconds(struct timeval t)
{
    return (double)t.tv_sec + (double)t.tv_usec / 1000000.0;
}