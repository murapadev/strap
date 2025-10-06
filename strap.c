#include "strap.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_MSC_VER)
#    include <intrin.h>
#endif

#if defined(__SSE2__) || (defined(_MSC_VER) && (defined(_M_X64) || defined(_M_AMD64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)))
#    define STRAP_HAVE_SSE2 1
#    include <emmintrin.h>
#else
#    define STRAP_HAVE_SSE2 0
#endif

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__) || defined(__linux__)
#    define STRAP_HAVE_TM_GMTOFF 1
#else
#    define STRAP_HAVE_TM_GMTOFF 0
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
#    include <xlocale.h>
#endif

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

#define STRAP_LINE_BUFFER_DEFAULT_CAPACITY 256

static unsigned char strap_ascii_tolower(unsigned char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return (unsigned char)(ch + ('a' - 'A'));
    return ch;
}

static void strap_split_free_partial(char **tokens, size_t count)
{
    if (!tokens)
        return;
    for (size_t i = 0; i < count; ++i)
        free(tokens[i]);
    free(tokens);
}

static int strap_split_reserve(char ***tokens_ptr, size_t *capacity, size_t needed, size_t max_tokens)
{
    if (needed <= *capacity)
        return 0;

    size_t new_capacity = *capacity;
    if (max_tokens > 0)
    {
        if (needed > max_tokens)
        {
            errno = EOVERFLOW;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return -1;
        }
        new_capacity = max_tokens;
    }
    else
    {
        if (new_capacity == 0)
            new_capacity = 4;
        while (new_capacity < needed)
        {
            if (new_capacity > SIZE_MAX / 2)
            {
                errno = EOVERFLOW;
                strap_set_error(STRAP_ERR_OVERFLOW);
                return -1;
            }
            new_capacity *= 2;
        }
    }

    if (strap_check_add_overflow(new_capacity, 1) ||
        strap_check_mul_overflow(new_capacity + 1, sizeof(char *)))
    {
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    char **resized = realloc(*tokens_ptr, (new_capacity + 1) * sizeof(char *));
    if (!resized)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return -1;
    }

    *tokens_ptr = resized;
    *capacity = new_capacity;
    return 0;
}

#if STRAP_HAVE_SSE2
static unsigned strap_ctz16(unsigned mask)
{
#    if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward(&idx, mask);
    return (unsigned)idx;
#    else
    return (unsigned)__builtin_ctz(mask);
#    endif
}

static unsigned strap_highest_bit_index16(unsigned mask)
{
#    if defined(_MSC_VER)
    unsigned long idx;
    _BitScanReverse(&idx, mask);
    return (unsigned)idx;
#    else
    return 31U - (unsigned)__builtin_clz(mask);
#    endif
}

static size_t strap_trim_leading_ascii_simd(const unsigned char *s, size_t len)
{
    const __m128i zero = _mm_setzero_si128();
    const __m128i space_limit = _mm_set1_epi8(' ');
    size_t offset = 0;

    while (offset + 16 <= len)
    {
        __m128i chunk = _mm_loadu_si128((const __m128i *)(s + offset));
        int non_ascii = _mm_movemask_epi8(_mm_cmplt_epi8(chunk, zero));
        if (non_ascii)
            return SIZE_MAX;

        __m128i above_space = _mm_cmpgt_epi8(chunk, space_limit);
        int stop_mask = _mm_movemask_epi8(above_space);
        if (stop_mask)
        {
            unsigned idx = strap_ctz16(stop_mask);
            return offset + idx;
        }
        offset += 16;
    }

    while (offset < len)
    {
        unsigned char c = s[offset];
        if (c > 0x7F)
            return SIZE_MAX;
        if (c > ' ')
            return offset;
        ++offset;
    }

    return len;
}

static size_t strap_trim_trailing_ascii_simd(const unsigned char *s, size_t len)
{
    if (len == 0)
        return 0;

    const __m128i zero = _mm_setzero_si128();
    const __m128i space_limit = _mm_set1_epi8(' ');
    size_t offset = len;

    while (offset >= 16)
    {
        size_t chunk_start = offset - 16;
        __m128i chunk = _mm_loadu_si128((const __m128i *)(s + chunk_start));
        int non_ascii = _mm_movemask_epi8(_mm_cmplt_epi8(chunk, zero));
        if (non_ascii)
            return SIZE_MAX;

        __m128i above_space = _mm_cmpgt_epi8(chunk, space_limit);
        int mask = _mm_movemask_epi8(above_space);
        if (mask)
        {
            unsigned idx = strap_highest_bit_index16(mask);
            return chunk_start + idx + 1;
        }
        offset -= 16;
    }

    while (offset > 0)
    {
        unsigned char c = s[offset - 1];
        if (c > 0x7F)
            return SIZE_MAX;
        if (c > ' ')
            return offset;
        --offset;
    }

    return 0;
}
#else
static size_t strap_trim_leading_ascii_simd(const unsigned char *s, size_t len)
{
    (void)s;
    (void)len;
    return SIZE_MAX;
}

static size_t strap_trim_trailing_ascii_simd(const unsigned char *s, size_t len)
{
    (void)s;
    (void)len;
    return SIZE_MAX;
}
#endif

static void strap_copy_bytes(char *dst, const char *src, size_t len)
{
    if (!dst || !src || len == 0)
        return;

#if STRAP_HAVE_SSE2
    size_t i = 0;
    for (; i + 16 <= len; i += 16)
    {
        __m128i chunk = _mm_loadu_si128((const __m128i *)(src + i));
        _mm_storeu_si128((__m128i *)(dst + i), chunk);
    }

    size_t remaining = len - i;
    if (remaining > 0)
        memcpy(dst + i, src + i, remaining);
#else
    memcpy(dst, src, len);
#endif
}

static int strap_line_buffer_reserve(strap_line_buffer_t *buffer, size_t needed)
{
    if (buffer->capacity >= needed)
        return 0;

    size_t new_capacity = buffer->capacity;
    if (new_capacity == 0)
        new_capacity = STRAP_LINE_BUFFER_DEFAULT_CAPACITY;

    while (new_capacity < needed)
    {
        if (new_capacity > SIZE_MAX / 2)
        {
            errno = EOVERFLOW;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return -1;
        }
        new_capacity *= 2;
    }

    char *resized = realloc(buffer->data, new_capacity);
    if (!resized)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return -1;
    }

    buffer->data = resized;
    buffer->capacity = new_capacity;
    return 0;
}

/* --------------------------------------------------------------------- */
/* Arena allocator internals                                            */

struct strap_arena_block
{
    struct strap_arena_block *next;
    size_t capacity;
    size_t used;
    unsigned char data[];
};

struct strap_arena
{
    struct strap_arena_block *head;
    size_t block_size;
};

static size_t strap_align_size(size_t value)
{
    const size_t alignment = sizeof(void *);
    size_t remainder = value % alignment;
    if (remainder == 0)
        return value;
    if (strap_check_add_overflow(value, alignment - remainder))
        return SIZE_MAX;
    return value + (alignment - remainder);
}

static struct strap_arena_block *strap_arena_new_block(size_t capacity)
{
    if (capacity == 0)
        capacity = 4096;

    size_t header_size = sizeof(struct strap_arena_block);
    if (strap_check_add_overflow(header_size, capacity))
    {
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return NULL;
    }

    size_t total = header_size + capacity;
    struct strap_arena_block *block = malloc(total);
    if (!block)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    block->next = NULL;
    block->capacity = capacity;
    block->used = 0;
    return block;
}

/* --------------------------------------------------------------------- */
/* Locale helpers                                                        */

typedef enum
{
    STRAP_LOCALE_NONE,
    STRAP_LOCALE_HANDLE,
    STRAP_LOCALE_GLOBAL
} strap_locale_kind_t;

typedef struct
{
    strap_locale_kind_t kind;
#if defined(_WIN32)
    _locale_t handle;
#else
    locale_t handle;
#endif
    char *saved_global;
} strap_locale_ctx;

static int strap_locale_enter(const char *locale_name, strap_locale_ctx *ctx)
{
    ctx->kind = STRAP_LOCALE_NONE;
    ctx->saved_global = NULL;

    if (!locale_name || locale_name[0] == '\0')
        return 0;

#if defined(_WIN32)
    ctx->handle = _create_locale(LC_ALL, locale_name);
    if (!ctx->handle)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }
    ctx->kind = STRAP_LOCALE_HANDLE;
    return 0;
#elif defined(LC_ALL_MASK)
    ctx->handle = newlocale(LC_ALL_MASK, locale_name, (locale_t)0);
    if (!ctx->handle)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }
    ctx->kind = STRAP_LOCALE_HANDLE;
    return 0;
#else
    const char *current = setlocale(LC_ALL, NULL);
    if (!current)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }
    ctx->saved_global = strdup(current);
    if (!ctx->saved_global)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return -1;
    }
    if (!setlocale(LC_ALL, locale_name))
    {
        free(ctx->saved_global);
        ctx->saved_global = NULL;
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }
    ctx->kind = STRAP_LOCALE_GLOBAL;
    return 0;
#endif
}

static void strap_locale_exit(strap_locale_ctx *ctx)
{
    if (!ctx)
        return;

    switch (ctx->kind)
    {
    case STRAP_LOCALE_NONE:
        break;
    case STRAP_LOCALE_HANDLE:
#if defined(_WIN32)
        if (ctx->handle)
            _free_locale(ctx->handle);
#else
        if (ctx->handle)
            freelocale(ctx->handle);
#endif
        break;
    case STRAP_LOCALE_GLOBAL:
        if (ctx->saved_global)
            setlocale(LC_ALL, ctx->saved_global);
        free(ctx->saved_global);
        ctx->saved_global = NULL;
        break;
    }

    ctx->kind = STRAP_LOCALE_NONE;
}

static int strap_tolower_locale_ctx(int ch, const strap_locale_ctx *ctx)
{
    unsigned char c = (unsigned char)ch;
    switch (ctx->kind)
    {
    case STRAP_LOCALE_HANDLE:
#if defined(_WIN32)
        return _tolower_l(c, ctx->handle);
#else
        return tolower_l(c, ctx->handle);
#endif
    case STRAP_LOCALE_GLOBAL:
    case STRAP_LOCALE_NONE:
    default:
        return tolower(c);
    }
}

static int strap_toupper_locale_ctx(int ch, const strap_locale_ctx *ctx)
{
    unsigned char c = (unsigned char)ch;
    switch (ctx->kind)
    {
    case STRAP_LOCALE_HANDLE:
#if defined(_WIN32)
        return _toupper_l(c, ctx->handle);
#else
        return toupper_l(c, ctx->handle);
#endif
    case STRAP_LOCALE_GLOBAL:
    case STRAP_LOCALE_NONE:
    default:
        return toupper(c);
    }
}


/* Safe reading */
void strap_line_buffer_init(strap_line_buffer_t *buffer)
{
    if (!buffer)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return;
    }

    buffer->data = NULL;
    buffer->capacity = 0;
    strap_clear_error();
}

void strap_line_buffer_free(strap_line_buffer_t *buffer)
{
    if (!buffer)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return;
    }

    free(buffer->data);
    buffer->data = NULL;
    buffer->capacity = 0;
}

char *strap_line_buffer_read(FILE *f, strap_line_buffer_t *buffer)
{
    if (!f || !buffer)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    size_t len = 0;
    if (buffer->capacity == 0)
    {
        if (strap_line_buffer_reserve(buffer, STRAP_LINE_BUFFER_DEFAULT_CAPACITY) != 0)
            return NULL;
    }
    else
    {
        buffer->data[0] = '\0';
    }

    for (;;)
    {
        if (buffer->capacity - len <= 1)
        {
            size_t target = len + STRAP_LINE_BUFFER_DEFAULT_CAPACITY;
            if (strap_line_buffer_reserve(buffer, target) != 0)
                return NULL;
        }

        if (!fgets(buffer->data + len, (int)(buffer->capacity - len), f))
            break;

        size_t chunk_len = strlen(buffer->data + len);
        char *newline = memchr(buffer->data + len, '\n', chunk_len);
        if (newline)
        {
            *newline = '\0';
            strap_clear_error();
            return buffer->data;
        }

        len += chunk_len;
    }

    if (len > 0 && buffer->data)
    {
        strap_clear_error();
        return buffer->data;
    }

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

char *afgets(FILE *f)
{
    if (!f)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    strap_line_buffer_t buffer;
    strap_line_buffer_init(&buffer);

    char *inplace = strap_line_buffer_read(f, &buffer);
    if (!inplace)
    {
        strap_line_buffer_free(&buffer);
        return NULL;
    }

    char *result = strdup(inplace);
    strap_line_buffer_free(&buffer);

    if (!result)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    strap_clear_error();
    return result;
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

/* Arena allocator */
strap_arena_t *strap_arena_create(size_t block_size)
{
    strap_arena_t *arena = malloc(sizeof(*arena));
    if (!arena)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    if (block_size == 0)
        block_size = 4096;

    arena->head = NULL;
    arena->block_size = block_size;
    strap_clear_error();
    return arena;
}

void strap_arena_destroy(strap_arena_t *arena)
{
    if (!arena)
        return;

    struct strap_arena_block *block = arena->head;
    while (block)
    {
        struct strap_arena_block *next = block->next;
        free(block);
        block = next;
    }

    free(arena);
}

void strap_arena_clear(strap_arena_t *arena)
{
    if (!arena)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return;
    }

    for (struct strap_arena_block *block = arena->head; block; block = block->next)
        block->used = 0;

    strap_clear_error();
}

void *strap_arena_alloc(strap_arena_t *arena, size_t size)
{
    if (!arena || size == 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    size_t aligned = strap_align_size(size);
    if (aligned == SIZE_MAX)
    {
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return NULL;
    }

    struct strap_arena_block *block = arena->head;
    if (!block || block->used + aligned > block->capacity)
    {
        size_t block_capacity = arena->block_size;
        if (aligned > block_capacity)
            block_capacity = aligned;

        struct strap_arena_block *new_block = strap_arena_new_block(block_capacity);
        if (!new_block)
            return NULL;

        new_block->next = arena->head;
        arena->head = new_block;
        block = new_block;
    }

    void *memory = block->data + block->used;
    block->used += aligned;
    strap_clear_error();
    return memory;
}

char *strap_arena_strdup(strap_arena_t *arena, const char *s)
{
    if (!arena || !s)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    size_t len = strlen(s);
    if (strap_check_add_overflow(len, 1))
    {
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return NULL;
    }

    char *dst = strap_arena_alloc(arena, len + 1);
    if (!dst)
        return NULL;

    memcpy(dst, s, len + 1);
    strap_clear_error();
    return dst;
}

char *strap_arena_strndup(strap_arena_t *arena, const char *s, size_t n)
{
    if (!arena || !s)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    if (strap_check_add_overflow(n, 1))
    {
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return NULL;
    }

    char *dst = strap_arena_alloc(arena, n + 1);
    if (!dst)
        return NULL;

    memcpy(dst, s, n);
    dst[n] = '\0';
    strap_clear_error();
    return dst;
}

/* String manipulation */
static char *strjoin_impl(strap_arena_t *arena, const char **parts, size_t nparts, const char *sep)
{
    if (!parts || nparts == 0)
    {
        if (arena)
        {
            char *empty = strap_arena_alloc(arena, 1);
            if (!empty)
                return NULL;
            empty[0] = '\0';
            strap_clear_error();
            return empty;
        }

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

    char *result;
    if (arena)
    {
        result = strap_arena_alloc(arena, total_len);
        if (!result)
        {
            free(lengths);
            return NULL;
        }
    }
    else
    {
        result = malloc(total_len);
        if (!result)
        {
            free(lengths);
            errno = ENOMEM;
            strap_set_error(STRAP_ERR_ALLOC);
            return NULL;
        }
    }

    char *write_ptr = result;
    for (size_t i = 0; i < nparts; ++i)
    {
        if (i > 0 && sep_len > 0)
        {
            strap_copy_bytes(write_ptr, sep, sep_len);
            write_ptr += sep_len;
        }

        size_t part_len = lengths[i];
        if (part_len > 0 && parts[i])
        {
            strap_copy_bytes(write_ptr, parts[i], part_len);
            write_ptr += part_len;
        }
    }

    *write_ptr = '\0';
    free(lengths);
    strap_clear_error();
    return result;
}

char *strjoin(const char **parts, size_t nparts, const char *sep)
{
    return strjoin_impl(NULL, parts, nparts, sep);
}

char *strjoin_arena(strap_arena_t *arena, const char **parts, size_t nparts, const char *sep)
{
    if (!arena)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }
    return strjoin_impl(arena, parts, nparts, sep);
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

    const char **parts_array = malloc(count * sizeof(char *));
    if (!parts_array)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    va_start(args, sep);
    for (size_t i = 0; i < count; ++i)
        parts_array[i] = va_arg(args, const char *);
    va_end(args);

    char *result = strjoin(parts_array, count, sep);
    free(parts_array);
    return result;
}

static int strap_strcasecmp_internal(const char *a, const char *b)
{
    const unsigned char *ua = (const unsigned char *)a;
    const unsigned char *ub = (const unsigned char *)b;

    while (*ua && *ub)
    {
        unsigned char ca = strap_ascii_tolower(*ua);
        unsigned char cb = strap_ascii_tolower(*ub);
        if (ca != cb)
            return (int)ca - (int)cb;
        ++ua;
        ++ub;
    }

    return (int)strap_ascii_tolower(*ua) - (int)strap_ascii_tolower(*ub);
}

int strap_strcasecmp(const char *a, const char *b)
{
    if (!a || !b)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return 0;
    }

    int diff = strap_strcasecmp_internal(a, b);
    strap_clear_error();
    return diff;
}

bool strcaseeq(const char *a, const char *b)
{
    if (!a || !b)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return false;
    }

    bool equal = (strap_strcasecmp_internal(a, b) == 0);
    strap_clear_error();
    return equal;
}

char **strsplit_limit(const char *s, const char *delim, size_t max_splits, size_t *out_count)
{
    if (!s || !delim)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    size_t delim_len = strlen(delim);
    if (delim_len == 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    size_t max_tokens = 0;
    if (max_splits > 0)
    {
        if (strap_check_add_overflow(max_splits, 1))
        {
            errno = EOVERFLOW;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return NULL;
        }
        max_tokens = max_splits + 1;
    }

    size_t capacity = max_tokens > 0 ? max_tokens : 4;
    if (strap_check_add_overflow(capacity, 1) ||
        strap_check_mul_overflow(capacity + 1, sizeof(char *)))
    {
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return NULL;
    }

    char **tokens = malloc((capacity + 1) * sizeof(char *));
    if (!tokens)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    size_t count = 0;
    size_t splits = 0;
    const char *cursor = s;

    for (;;)
    {
        if (max_splits > 0 && splits >= max_splits)
        {
            char *tail = strdup(cursor);
            if (!tail)
            {
                errno = ENOMEM;
                strap_set_error(STRAP_ERR_ALLOC);
                strap_split_free_partial(tokens, count);
                return NULL;
            }

            if (strap_split_reserve(&tokens, &capacity, count + 1, max_tokens) != 0)
            {
                free(tail);
                strap_split_free_partial(tokens, count);
                return NULL;
            }

            tokens[count++] = tail;
            break;
        }

        const char *match = strstr(cursor, delim);
        if (!match)
        {
            size_t tail_len = strlen(cursor);
            char *tail = malloc(tail_len + 1);
            if (!tail)
            {
                errno = ENOMEM;
                strap_set_error(STRAP_ERR_ALLOC);
                strap_split_free_partial(tokens, count);
                return NULL;
            }
            memcpy(tail, cursor, tail_len + 1);

            if (strap_split_reserve(&tokens, &capacity, count + 1, max_tokens) != 0)
            {
                free(tail);
                strap_split_free_partial(tokens, count);
                return NULL;
            }

            tokens[count++] = tail;
            break;
        }

        size_t segment_len = (size_t)(match - cursor);
        char *segment = malloc(segment_len + 1);
        if (!segment)
        {
            errno = ENOMEM;
            strap_set_error(STRAP_ERR_ALLOC);
            strap_split_free_partial(tokens, count);
            return NULL;
        }

        if (segment_len > 0)
            memcpy(segment, cursor, segment_len);
        segment[segment_len] = '\0';

        if (strap_split_reserve(&tokens, &capacity, count + 1, max_tokens) != 0)
        {
            free(segment);
            strap_split_free_partial(tokens, count);
            return NULL;
        }

        tokens[count++] = segment;
        cursor = match + delim_len;
        ++splits;
    }

    if (strap_split_reserve(&tokens, &capacity, count, max_tokens) != 0)
    {
        strap_split_free_partial(tokens, count);
        return NULL;
    }

    tokens[count] = NULL;
    if (out_count)
        *out_count = count;

    strap_clear_error();
    return tokens;
}

char **strsplit_predicate(const char *s,
                          strap_split_predicate_fn predicate,
                          void *userdata,
                          size_t max_splits,
                          size_t *out_count)
{
    if (!s || !predicate)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    size_t max_tokens = 0;
    if (max_splits > 0)
    {
        if (strap_check_add_overflow(max_splits, 1))
        {
            errno = EOVERFLOW;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return NULL;
        }
        max_tokens = max_splits + 1;
    }

    size_t capacity = max_tokens > 0 ? max_tokens : 4;
    if (strap_check_add_overflow(capacity, 1) ||
        strap_check_mul_overflow(capacity + 1, sizeof(char *)))
    {
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return NULL;
    }

    char **tokens = malloc((capacity + 1) * sizeof(char *));
    if (!tokens)
    {
        errno = ENOMEM;
        strap_set_error(STRAP_ERR_ALLOC);
        return NULL;
    }

    size_t count = 0;
    size_t splits = 0;
    size_t pos = 0;
    const unsigned char *buffer = (const unsigned char *)s;

    while (buffer[pos] != '\0')
    {
        while (buffer[pos] != '\0' && predicate(buffer[pos], userdata))
            ++pos;
        if (buffer[pos] == '\0')
            break;

        if (max_splits > 0 && splits >= max_splits)
        {
            char *tail = strdup((const char *)&buffer[pos]);
            if (!tail)
            {
                errno = ENOMEM;
                strap_set_error(STRAP_ERR_ALLOC);
                strap_split_free_partial(tokens, count);
                return NULL;
            }

            if (strap_split_reserve(&tokens, &capacity, count + 1, max_tokens) != 0)
            {
                free(tail);
                strap_split_free_partial(tokens, count);
                return NULL;
            }

            tokens[count++] = tail;
            pos += strlen((const char *)&buffer[pos]);
            break;
        }

        size_t start = pos;
        while (buffer[pos] != '\0' && !predicate(buffer[pos], userdata))
            ++pos;

        size_t token_len = pos - start;
        char *token = malloc(token_len + 1);
        if (!token)
        {
            errno = ENOMEM;
            strap_set_error(STRAP_ERR_ALLOC);
            strap_split_free_partial(tokens, count);
            return NULL;
        }

        if (token_len > 0)
            memcpy(token, &buffer[start], token_len);
        token[token_len] = '\0';

        if (strap_split_reserve(&tokens, &capacity, count + 1, max_tokens) != 0)
        {
            free(token);
            strap_split_free_partial(tokens, count);
            return NULL;
        }

        tokens[count++] = token;

        if (buffer[pos] == '\0')
            break;

        ++splits;
    }

    tokens[count] = NULL;
    if (out_count)
        *out_count = count;

    strap_clear_error();
    return tokens;
}

void strsplit_free(char **tokens)
{
    if (!tokens)
        return;
    for (size_t i = 0; tokens[i] != NULL; ++i)
        free(tokens[i]);
    free(tokens);
}

/* Trim */
static char *strtrim_impl(strap_arena_t *arena, const char *s)
{
    if (!s)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    const unsigned char *bytes = (const unsigned char *)s;
    size_t total_len = strlen(s);

    size_t leading = strap_trim_leading_ascii_simd(bytes, total_len);
    const unsigned char *start = bytes;

    if (leading == SIZE_MAX)
    {
        leading = 0;
        while (leading < total_len && isspace((unsigned char)start[0]))
        {
            ++start;
            ++leading;
        }
    }
    else
    {
        start += leading;
    }

    size_t remaining_len = total_len - leading;
    size_t trimmed_len = strap_trim_trailing_ascii_simd(start, remaining_len);
    size_t len;

    if (trimmed_len == SIZE_MAX)
    {
        len = remaining_len;
        while (len > 0 && isspace((unsigned char)start[len - 1]))
            --len;
    }
    else
    {
        len = trimmed_len;
    }

    if (strap_check_add_overflow(len, 1))
    {
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return NULL;
    }

    char *result;
    if (arena)
    {
        result = strap_arena_alloc(arena, len + 1);
        if (!result)
            return NULL;
    }
    else
    {
        result = malloc(len + 1);
        if (!result)
        {
            errno = ENOMEM;
            strap_set_error(STRAP_ERR_ALLOC);
            return NULL;
        }
    }

    if (len > 0)
        memcpy(result, start, len);
    result[len] = '\0';

    strap_clear_error();
    return result;
}

char *strtrim(const char *s)
{
    return strtrim_impl(NULL, s);
}

char *strtrim_arena(strap_arena_t *arena, const char *s)
{
    if (!arena)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }
    return strtrim_impl(arena, s);
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

static char *strreplace_impl(strap_arena_t *arena, const char *s, const char *search, const char *replacement)
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
        if (arena)
            return strap_arena_strdup(arena, s);

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

    char *result;
    if (arena)
    {
        result = strap_arena_alloc(arena, total_len + 1);
        if (!result)
            return NULL;
    }
    else
    {
        result = malloc(total_len + 1);
        if (!result)
        {
            errno = ENOMEM;
            strap_set_error(STRAP_ERR_ALLOC);
            return NULL;
        }
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

char *strreplace(const char *s, const char *search, const char *replacement)
{
    return strreplace_impl(NULL, s, search, replacement);
}

char *strreplace_arena(strap_arena_t *arena, const char *s, const char *search, const char *replacement)
{
    if (!arena)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }
    return strreplace_impl(arena, s, search, replacement);
}

static char *strap_locale_case_impl(strap_arena_t *arena, const char *s, const char *locale_name, int make_upper)
{
    if (!s)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }

    strap_locale_ctx ctx;
    if (strap_locale_enter(locale_name, &ctx) != 0)
        return NULL;

    size_t len = strlen(s);
    if (strap_check_add_overflow(len, 1))
    {
        strap_locale_exit(&ctx);
        errno = EOVERFLOW;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return NULL;
    }

    char *buffer;
    if (arena)
    {
        buffer = strap_arena_alloc(arena, len + 1);
        if (!buffer)
        {
            strap_locale_exit(&ctx);
            return NULL;
        }
    }
    else
    {
        buffer = malloc(len + 1);
        if (!buffer)
        {
            strap_locale_exit(&ctx);
            errno = ENOMEM;
            strap_set_error(STRAP_ERR_ALLOC);
            return NULL;
        }
    }

    for (size_t i = 0; i < len; ++i)
    {
        unsigned char ch = (unsigned char)s[i];
        int converted = make_upper ? strap_toupper_locale_ctx(ch, &ctx) : strap_tolower_locale_ctx(ch, &ctx);
        buffer[i] = (char)converted;
    }
    buffer[len] = '\0';

    strap_locale_exit(&ctx);
    strap_clear_error();
    return buffer;
}

char *strtolower_locale(const char *s, const char *locale_name)
{
    return strap_locale_case_impl(NULL, s, locale_name, 0);
}

char *strtolower_locale_arena(strap_arena_t *arena, const char *s, const char *locale_name)
{
    if (!arena)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }
    return strap_locale_case_impl(arena, s, locale_name, 0);
}

char *strtoupper_locale(const char *s, const char *locale_name)
{
    return strap_locale_case_impl(NULL, s, locale_name, 1);
}

char *strtoupper_locale_arena(strap_arena_t *arena, const char *s, const char *locale_name)
{
    if (!arena)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return NULL;
    }
    return strap_locale_case_impl(arena, s, locale_name, 1);
}

int strcoll_locale(const char *a, const char *b, const char *locale_name)
{
    if (!a || !b)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return 0;
    }

    strap_locale_ctx ctx;
    if (strap_locale_enter(locale_name, &ctx) != 0)
        return 0;

    int cmp;
    if (ctx.kind == STRAP_LOCALE_HANDLE)
    {
#if defined(_WIN32)
        cmp = _strcoll_l(a, b, ctx.handle);
#else
        cmp = strcoll_l(a, b, ctx.handle);
#endif
    }
    else
    {
        cmp = strcoll(a, b);
    }

    strap_locale_exit(&ctx);
    strap_clear_error();
    return cmp;
}

int strcasecmp_locale(const char *a, const char *b, const char *locale_name)
{
    if (!a || !b)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return 0;
    }

    strap_locale_ctx ctx;
    if (strap_locale_enter(locale_name, &ctx) != 0)
        return 0;

    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;

    while (*pa && *pb)
    {
        int ca = strap_tolower_locale_ctx(*pa, &ctx);
        int cb = strap_tolower_locale_ctx(*pb, &ctx);
        if (ca != cb)
        {
            strap_locale_exit(&ctx);
            strap_clear_error();
            return ca - cb;
        }
        ++pa;
        ++pb;
    }

    int tail_a = strap_tolower_locale_ctx(*pa, &ctx);
    int tail_b = strap_tolower_locale_ctx(*pb, &ctx);
    int diff = tail_a - tail_b;

    strap_locale_exit(&ctx);
    strap_clear_error();
    return diff;
}

/* --------------------------------------------------------------------- */
/* Time helpers                                                           */

static int strap_gmtime_safe(time_t value, struct tm *out)
{
    if (!out)
        return -1;

#if defined(_WIN32)
    return gmtime_s(out, &value) == 0 ? 0 : -1;
#elif defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__unix__) || defined(__APPLE__)
    return gmtime_r(&value, out) ? 0 : -1;
#else
    struct tm *tmp = gmtime(&value);
    if (!tmp)
        return -1;
    *out = *tmp;
    return 0;
#endif
}

static int strap_localtime_safe(time_t value, struct tm *out)
{
    if (!out)
        return -1;

#if defined(_WIN32)
    return localtime_s(out, &value) == 0 ? 0 : -1;
#elif defined(_POSIX_THREAD_SAFE_FUNCTIONS) || defined(__unix__) || defined(__APPLE__)
    return localtime_r(&value, out) ? 0 : -1;
#else
    struct tm *tmp = localtime(&value);
    if (!tmp)
        return -1;
    *out = *tmp;
    return 0;
#endif
}

static int strap_is_leap(int year)
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

static int strap_days_in_month(int year, unsigned month)
{
    static const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12)
        return 0;
    if (month == 2 && strap_is_leap(year))
        return 29;
    return days[month - 1];
}

static int64_t strap_days_from_civil(int year, unsigned month, unsigned day)
{
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + (int64_t)doe - 719468;
}

static int strap_tm_to_epoch(const struct tm *tm_utc, int64_t *out_seconds)
{
    if (!tm_utc || !out_seconds)
        return -1;

    int year = tm_utc->tm_year + 1900;
    unsigned month = (unsigned)(tm_utc->tm_mon + 1);
    unsigned day = (unsigned)tm_utc->tm_mday;
    int hour = tm_utc->tm_hour;
    int minute = tm_utc->tm_min;
    int second = tm_utc->tm_sec;

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60)
        return -1;

    int dim = strap_days_in_month(year, month);
    if (dim == 0 || day == 0 || day > (unsigned)dim)
        return -1;

    int64_t days = strap_days_from_civil(year, month, day);

    int64_t seconds_from_days;
    if ((days > 0 && days > INT64_MAX / 86400) || (days < 0 && days < INT64_MIN / 86400))
        return -1;
    seconds_from_days = days * 86400;

    int64_t day_seconds = (int64_t)hour * 3600 + (int64_t)minute * 60 + (int64_t)second;
    if ((day_seconds > 0 && seconds_from_days > INT64_MAX - day_seconds) ||
        (day_seconds < 0 && seconds_from_days < INT64_MIN - day_seconds))
        return -1;

    *out_seconds = seconds_from_days + day_seconds;
    return 0;
}

static int strap_apply_offset(int64_t base, int offset_minutes, int64_t *out_seconds)
{
    int64_t delta = (int64_t)offset_minutes * 60;
    if ((delta > 0 && base > INT64_MAX - delta) || (delta < 0 && base < INT64_MIN - delta))
        return -1;
    *out_seconds = base + delta;
    return 0;
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

struct timeval timeval_add_minutes(struct timeval t, int minutes)
{
    struct timeval result = t;
    int64_t sec = (int64_t)t.tv_sec + (int64_t)minutes * 60;
    result.tv_sec = (time_t)sec;
    return result;
}

int strap_time_offset_to_string(int offset_minutes, char *buf, size_t bufsize)
{
    if (!buf)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    if (offset_minutes == 0)
    {
        if (bufsize < 2)
        {
            errno = ERANGE;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return -1;
        }
        buf[0] = 'Z';
        buf[1] = '\0';
        strap_clear_error();
        return 0;
    }

    if (offset_minutes < -14 * 60 || offset_minutes > 14 * 60)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    if (bufsize < 7)
    {
        errno = ERANGE;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    int sign = offset_minutes < 0 ? -1 : 1;
    int total = offset_minutes * sign;
    int hours = total / 60;
    int minutes = total % 60;

    if (hours == 14 && minutes != 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    snprintf(buf, bufsize, "%c%02d:%02d", sign < 0 ? '-' : '+', hours, minutes);
    strap_clear_error();
    return 0;
}

int strap_time_parse_tz_offset(const char *str, int *offset_minutes)
{
    if (!str || !offset_minutes)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    if ((str[0] == 'Z' || str[0] == 'z') && str[1] == '\0')
    {
        *offset_minutes = 0;
        strap_clear_error();
        return 0;
    }

    int sign;
    if (str[0] == '+')
        sign = 1;
    else if (str[0] == '-')
        sign = -1;
    else
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    const char *p = str + 1;
    if (!isdigit((unsigned char)p[0]) || !isdigit((unsigned char)p[1]))
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    int hours = (p[0] - '0') * 10 + (p[1] - '0');
    p += 2;

    int minutes = 0;
    if (*p == ':')
    {
        ++p;
        if (!isdigit((unsigned char)p[0]) || !isdigit((unsigned char)p[1]))
        {
            errno = EINVAL;
            strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
            return -1;
        }
        minutes = (p[0] - '0') * 10 + (p[1] - '0');
        p += 2;
    }
    else if (isdigit((unsigned char)p[0]) && isdigit((unsigned char)p[1]))
    {
        minutes = (p[0] - '0') * 10 + (p[1] - '0');
        p += 2;
    }
    else if (*p == '\0')
    {
        minutes = 0;
    }
    else
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    if (*p != '\0')
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    if (hours > 14 || minutes >= 60 || (hours == 14 && minutes != 0))
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    *offset_minutes = sign * (hours * 60 + minutes);
    strap_clear_error();
    return 0;
}

int strap_time_format_iso8601(struct timeval t, int offset_minutes, char *buf, size_t bufsize)
{
    if (!buf)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    char tzbuf[7];
    if (strap_time_offset_to_string(offset_minutes, tzbuf, sizeof(tzbuf)) != 0)
        return -1;

    int64_t local_seconds;
    if (strap_apply_offset((int64_t)t.tv_sec, offset_minutes, &local_seconds) != 0)
    {
        errno = ERANGE;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    time_t local_time = (time_t)local_seconds;
    if ((int64_t)local_time != local_seconds)
    {
        errno = ERANGE;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    struct tm tm_local;
    if (strap_gmtime_safe(local_time, &tm_local) != 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    int written = snprintf(buf, bufsize, "%04d-%02d-%02dT%02d:%02d:%02d",
                           tm_local.tm_year + 1900,
                           tm_local.tm_mon + 1,
                           tm_local.tm_mday,
                           tm_local.tm_hour,
                           tm_local.tm_min,
                           tm_local.tm_sec);
    if (written < 0 || (size_t)written >= bufsize)
    {
        errno = ERANGE;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    size_t pos = (size_t)written;

    if (t.tv_usec > 0)
    {
        int frac = snprintf(buf + pos, bufsize - pos, ".%06ld", (long)t.tv_usec);
        if (frac < 0 || (size_t)frac >= bufsize - pos)
        {
            errno = ERANGE;
            strap_set_error(STRAP_ERR_OVERFLOW);
            return -1;
        }
        pos += (size_t)frac;
    }

    int tz_written = snprintf(buf + pos, bufsize - pos, "%s", tzbuf);
    if (tz_written < 0 || (size_t)tz_written >= bufsize - pos)
    {
        errno = ERANGE;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    strap_clear_error();
    return 0;
}

int strap_time_local_offset(time_t when, int *offset_minutes)
{
    if (!offset_minutes)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    struct tm local_tm;
    if (strap_localtime_safe(when, &local_tm) != 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    int64_t offset_seconds;

#if STRAP_HAVE_TM_GMTOFF
    offset_seconds = (int64_t)local_tm.tm_gmtoff;
#else
    struct tm probe = local_tm;
    time_t local_epoch = mktime(&probe);
    if (local_epoch == (time_t)-1)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }
    offset_seconds = (int64_t)local_epoch - (int64_t)when;
#endif

    if (offset_seconds < -14LL * 3600 || offset_seconds > 14LL * 3600)
    {
        errno = ERANGE;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    if (offset_seconds % 60 != 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    int candidate = (int)(offset_seconds / 60);
    if ((int64_t)candidate != offset_seconds / 60)
    {
        errno = ERANGE;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    *offset_minutes = candidate;
    strap_clear_error();
    return 0;
}

int strap_time_format_iso8601_local(struct timeval t, char *buf, size_t bufsize)
{
    int offset_minutes;
    if (strap_time_local_offset(t.tv_sec, &offset_minutes) != 0)
        return -1;
    return strap_time_format_iso8601(t, offset_minutes, buf, bufsize);
}

static int strap_parse_fixed_digits(const char **cursor, size_t count, int *out_value)
{
    int value = 0;
    const char *p = *cursor;
    for (size_t i = 0; i < count; ++i)
    {
        if (!isdigit((unsigned char)p[i]))
            return -1;
        value = value * 10 + (p[i] - '0');
    }
    *cursor = p + count;
    *out_value = value;
    return 0;
}

int strap_time_parse_iso8601(const char *str, struct timeval *out, int *offset_minutes)
{
    if (!str || !out)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    const char *p = str;
    int year, month, day, hour, minute, second;

    if (strap_parse_fixed_digits(&p, 4, &year) != 0 || *p++ != '-' ||
        strap_parse_fixed_digits(&p, 2, &month) != 0 || *p++ != '-' ||
        strap_parse_fixed_digits(&p, 2, &day) != 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    if (*p != 'T' && *p != 't' && *p != ' ')
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }
    ++p;

    if (strap_parse_fixed_digits(&p, 2, &hour) != 0 || *p++ != ':' ||
        strap_parse_fixed_digits(&p, 2, &minute) != 0 || *p++ != ':' ||
        strap_parse_fixed_digits(&p, 2, &second) != 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    int micro = 0;
    if (*p == '.' || *p == ',')
    {
        ++p;
        const char *frac_start = p;
        size_t digits = 0;
        int value = 0;
        while (isdigit((unsigned char)*p) && digits < 6)
        {
            value = value * 10 + (*p - '0');
            ++p;
            ++digits;
        }

        if (!isdigit((unsigned char)*frac_start) || digits == 0)
        {
            errno = EINVAL;
            strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
            return -1;
        }

        if (isdigit((unsigned char)*p))
        {
            errno = EINVAL;
            strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
            return -1;
        }

        while (digits++ < 6)
            value *= 10;
        micro = value;
    }

    if (*p == '\0')
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    int parsed_offset;
    if (strap_time_parse_tz_offset(p, &parsed_offset) != 0)
        return -1;

    struct tm tm_local;
    memset(&tm_local, 0, sizeof(tm_local));
    tm_local.tm_year = year - 1900;
    tm_local.tm_mon = month - 1;
    tm_local.tm_mday = day;
    tm_local.tm_hour = hour;
    tm_local.tm_min = minute;
    tm_local.tm_sec = second;

    int64_t local_seconds;
    if (strap_tm_to_epoch(&tm_local, &local_seconds) != 0)
    {
        errno = EINVAL;
        strap_set_error(STRAP_ERR_INVALID_ARGUMENT);
        return -1;
    }

    int64_t utc_seconds;
    if (strap_apply_offset(local_seconds, -parsed_offset, &utc_seconds) != 0)
    {
        errno = ERANGE;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    time_t utc_time = (time_t)utc_seconds;
    if ((int64_t)utc_time != utc_seconds)
    {
        errno = ERANGE;
        strap_set_error(STRAP_ERR_OVERFLOW);
        return -1;
    }

    out->tv_sec = utc_time;
    out->tv_usec = micro;

    if (offset_minutes)
        *offset_minutes = parsed_offset;

    strap_clear_error();
    return 0;
}
