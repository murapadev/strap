#include "strap.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER)
#    define STRAP_THREAD_LOCAL __declspec(thread)
#else
#    define STRAP_THREAD_LOCAL _Thread_local
#endif

static STRAP_THREAD_LOCAL strap_error_t strap_global_error = STRAP_OK;

static void strap_set_error(strap_error_t err)
{
    strap_global_error = err;
}

strap_error_t strap_last_error(void)
{
    return strap_global_error;
}

const char *strap_error_string(strap_error_t err)
{
    switch (err)
    {
    case STRAP_OK:
        return "no error";
    case STRAP_ERR_INVALID_ARGUMENT:
        return "invalid argument";
    case STRAP_ERR_ALLOC:
        return "allocation failed";
    case STRAP_ERR_IO:
        return "I/O error";
    case STRAP_ERR_OVERFLOW:
        return "size overflow";
    default:
        return "unknown error";
    }
}

void strap_clear_error(void)
{
    strap_global_error = STRAP_OK;
}

static int strap_check_add_overflow(size_t a, size_t b)
{
    return b > 0 && a > SIZE_MAX - b;
}

static int strap_check_mul_overflow(size_t a, size_t b)
{
    return (a != 0 && b > SIZE_MAX / a);
}

/* Safe reading */
char *afgets(FILE *f)
{
    if (!f)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    size_t capacity = 128;
    size_t len = 0;
    char *line = malloc(capacity);
    if (!line)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    for (;;)
    {
        if (capacity - len <= 1)
        {
            if (capacity > SIZE_MAX / 2)
            {
                free(line);
                errno = EOVERFLOW;
                strap_set_error(STRAP_ERR_OVERFLOW);
                return NULL;
            }
            size_t new_capacity = capacity * 2;
            char *tmp = realloc(line, new_capacity);
            if (!tmp)
            {
                free(line);
                errno = ENOMEM;
                strap_set_error(STRAP_ERR_ALLOC);
                return NULL;
            }
            line = tmp;
            capacity = new_capacity;
        }

        if (!fgets(line + len, (int)(capacity - len), f))
            break;

        size_t chunk_len = strlen(line + len);
        char *newline = memchr(line + len, '\n', chunk_len);
        if (newline)
        {
            *newline = '\0';
            strap_clear_error();
            return line;
        }

        len += chunk_len;

        if (feof(f))
        {
            strap_clear_error();
            return line;
        }
    }

    if (len > 0)
    {
        strap_clear_error();
        return line;
    }

    free(line);

    if (ferror(f))
    {
        errno = EIO;
        strap_set_error(STRAP_ERR_IO);
    }
    else
    {
        strap_clear_error();
    }

    return NULL;
}

char *afread(FILE *f, size_t *out_len)
{
    if (!f)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    const size_t chunk = 4096;
    size_t capacity = chunk;
    size_t len = 0;

    char *buffer = malloc(capacity + 1);
    if (!buffer)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    for (;;)
    {
        if (len == capacity)
        {
            if (strap_check_add_overflow(capacity, chunk))
            {
                free(buffer);
                errno = EOVERFLOW;
                strap_set_error(STRAP_ERR_OVERFLOW);
                return NULL;
            }
            size_t new_capacity = capacity + chunk;
            char *tmp = realloc(buffer, new_capacity + 1);
            if (!tmp)
            {
                free(buffer);
                errno = ENOMEM;
                strap_set_error(STRAP_ERR_ALLOC);
                return NULL;
            }
            buffer = tmp;
            capacity = new_capacity;
        }

        size_t to_read = capacity - len;
        size_t n = fread(buffer + len, 1, to_read, f);
        len += n;

        if (n < to_read)
        {
            if (feof(f))
                break;

            if (ferror(f))
            {
                free(buffer);
                errno = EIO;
                strap_set_error(STRAP_ERR_IO);
                return NULL;
            }
        }
    }

    buffer[len] = '\0';

    char *shrunk = realloc(buffer, len + 1);
    if (shrunk)
        buffer = shrunk;

    if (out_len)
        *out_len = len;

    strap_clear_error();
    return buffer;
}

/* String manipulation */
char *strjoin(const char **parts, size_t nparts, const char *sep)
{
    if (!parts || nparts == 0)
    {
        char *empty = strdup("");
        if (!empty)
        {
            errno = ENOMEM;
            strap_set_error(STRAP_ERR_ALLOC);
        }
        else
        {
            strap_clear_error();
        }
        return empty;
    }

    size_t sep_len = sep ? strlen(sep) : 0;
    size_t *lengths = malloc(nparts * sizeof(size_t));
    if (!lengths)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    size_t total_len = 1; /* null terminator */

    for (size_t i = 0; i < nparts; ++i)
    {
        size_t part_len = parts[i] ? strlen(parts[i]) : 0;
        lengths[i] = part_len;

        if (strap_check_add_overflow(total_len, part_len))
        {
            free(lengths);
            errno = EOVERFLOW;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return NULL;
        }
        total_len += part_len;

        if (i + 1 < nparts && sep_len > 0)
        {
            if (strap_check_add_overflow(total_len, sep_len))
            {
                free(lengths);
                errno = EOVERFLOW;
                strap_set_error(STRAP_ERR_OVERFLOW);
                return NULL;
            }
            total_len += sep_len;
        }
    }

    char *result = malloc(total_len);
    if (!result)
    {
        free(lengths);
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    char *write_ptr = result;
    for (size_t i = 0; i < nparts; ++i)
    {
        if (i > 0 && sep_len > 0)
        {
            memcpy(write_ptr, sep, sep_len);
            write_ptr += sep_len;
        }

        size_t part_len = lengths[i];
        if (part_len > 0 && parts[i])
        {
            memcpy(write_ptr, parts[i], part_len);
            write_ptr += part_len;
        }
    }

    *write_ptr = '\0';
    free(lengths);
    strap_clear_error();
    return result;
}

char *strjoin_va(const char *sep, ...)
{
    va_list args;
    va_start(args, sep);

    size_t count = 0;
    const char *part;
    while ((part = va_arg(args, const char *)) != NULL)
        ++count;
    va_end(args);

    if (count == 0)
    {
        char *empty = strdup("");
        if (!empty)
        {
            errno = ENOMEM;
            strap_set_error(STRAP_ERR_ALLOC);
        }
        else
        {
            strap_clear_error();
        }
        return empty;
    }

    const char **parts = malloc(count * sizeof(char *));
    if (!parts)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    va_start(args, sep);
    for (size_t i = 0; i < count; ++i)
        parts[i] = va_arg(args, const char *);
    va_end(args);

    char *result = strjoin(parts, count, sep);
    free(parts);
    return result;
}

/* Trim */
char *strtrim(const char *s)
{
    if (!s)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    const unsigned char *start = (const unsigned char *)s;
    while (*start && isspace(*start))
        ++start;

    size_t len = strlen((const char *)start);
    while (len > 0 && isspace(start[len - 1]))
        --len;

    char *result = malloc(len + 1);
    if (!result)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    if (len > 0)
        memcpy(result, start, len);
    result[len] = '\0';

    strap_clear_error();
    return result;
}

void strtrim_inplace(char *s)
{
    if (!s)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return;
    }

    unsigned char *start = (unsigned char *)s;
    while (*start && isspace(*start))
        ++start;

    if ((char *)start != s)
        memmove(s, start, strlen((const char *)start) + 1);

    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1]))
    {
        s[len - 1] = '\0';
        --len;
    }

    strap_clear_error();
}

bool strstartswith(const char *s, const char *prefix)
{
    if (!s || !prefix)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return false;
    }

    size_t prefix_len = strlen(prefix);
    bool matches = strncmp(s, prefix, prefix_len) == 0;
    strap_clear_error();
    return matches;
}

bool strendswith(const char *s, const char *suffix)
{
    if (!s || !suffix)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return false;
    }

    size_t s_len = strlen(s);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > s_len)
    {
        strap_clear_error();
        return false;
    }

    bool matches = strncmp(s + (s_len - suffix_len), suffix, suffix_len) == 0;
    strap_clear_error();
    return matches;
}

char *strreplace(const char *s, const char *search, const char *replacement)
{
    if (!s || !search)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    if (!replacement)
        replacement = "";

    size_t search_len = strlen(search);
    if (search_len == 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    size_t replace_len = strlen(replacement);
    size_t count = 0;

    const char *cursor = s;
    while ((cursor = strstr(cursor, search)) != NULL)
    {
        ++count;
        cursor += search_len;
    }

    if (count == 0)
    {
        char *copy = strdup(s);
        if (!copy)
        {
            errno = ENOMEM;
            strap_set_error(STRAP_ERR_ALLOC);
        }
        else
        {
            strap_clear_error();
        }
        return copy;
    }

    size_t base_len = strlen(s);
    size_t total_len = base_len;

    if (replace_len >= search_len)
    {
        size_t diff = replace_len - search_len;
        if (strap_check_mul_overflow(count, diff))
        {
            errno = EOVERFLOW;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return NULL;
        }
        size_t increase = count * diff;
        if (strap_check_add_overflow(total_len, increase))
        {
            errno = EOVERFLOW;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return NULL;
        }
        total_len += increase;
    }
    else
    {
        size_t diff = search_len - replace_len;
        size_t decrease = count * diff;
        if (decrease > total_len)
        {
            errno = EOVERFLOW;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return NULL;
        }
        total_len -= decrease;
    }

    if (strap_check_add_overflow(total_len, 1))
    {
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return NULL;
    }

    char *result = malloc(total_len + 1);
    if (!result)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    const char *src = s;
    char *dst = result;
    const char *match;

    while ((match = strstr(src, search)) != NULL)
    {
        size_t segment_len = (size_t)(match - src);
        memcpy(dst, src, segment_len);
        dst += segment_len;
        if (replace_len > 0)
        {
            memcpy(dst, replacement, replace_len);
            dst += replace_len;
        }
        src = match + search_len;
    }

    size_t tail_len = strlen(src);
    memcpy(dst, src, tail_len);
    dst += tail_len;
    *dst = '\0';

    strap_clear_error();
    return result;
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
