/* strap.h - initial version */
#ifndef STRAP_H
#define STRAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/time.h>

typedef enum
{
    STRAP_OK = 0,
    STRAP_ERR_INVALID_ARGUMENT,
    STRAP_ERR_ALLOC,
    STRAP_ERR_IO,
    STRAP_ERR_OVERFLOW
} strap_error_t;

typedef struct strap_arena strap_arena_t;

strap_error_t strap_last_error(void);
const char *strap_error_string(strap_error_t err);
void strap_clear_error(void);

/* Safe reading */
char *afgets(FILE *f);                  /* reads a complete line, returns malloc() buffer or NULL */
char *afread(FILE *f, size_t *out_len); /* reads entire file into heap, returns buffer and length */

/* String manipulation */
char *strjoin(const char **parts, size_t nparts, const char *sep); /* returns malloc() */
char *strjoin_va(const char *sep, ...);                            /* varargs, ends with NULL */
bool strstartswith(const char *s, const char *prefix);
bool strendswith(const char *s, const char *suffix);
char *strreplace(const char *s, const char *search, const char *replacement);
char *strtolower_locale(const char *s, const char *locale_name);      /* malloc(), optional locale */
char *strtoupper_locale(const char *s, const char *locale_name);      /* malloc(), optional locale */
int strcoll_locale(const char *a, const char *b, const char *locale_name);
int strcasecmp_locale(const char *a, const char *b, const char *locale_name);

/* Trim (inplace or return new) */
char *strtrim(const char *s);  /* returns new malloc() without spaces at start/end */
void strtrim_inplace(char *s); /* modifies buffer in-place */
char *strtrim_arena(strap_arena_t *arena, const char *s);

/* Arena allocator */
strap_arena_t *strap_arena_create(size_t block_size);
void strap_arena_destroy(strap_arena_t *arena);
void strap_arena_clear(strap_arena_t *arena);
void *strap_arena_alloc(strap_arena_t *arena, size_t size);
char *strap_arena_strdup(strap_arena_t *arena, const char *s);
char *strap_arena_strndup(strap_arena_t *arena, const char *s, size_t n);
char *strjoin_arena(strap_arena_t *arena, const char **parts, size_t nparts, const char *sep);
char *strreplace_arena(strap_arena_t *arena, const char *s, const char *search, const char *replacement);
char *strtolower_locale_arena(strap_arena_t *arena, const char *s, const char *locale_name);
char *strtoupper_locale_arena(strap_arena_t *arena, const char *s, const char *locale_name);

/* Time utilities (struct timeval) */
struct timeval timeval_add(struct timeval a, struct timeval b);
struct timeval timeval_sub(struct timeval a, struct timeval b);
double timeval_to_seconds(struct timeval t);
struct timeval timeval_add_minutes(struct timeval t, int minutes);
int strap_time_offset_to_string(int offset_minutes, char *buf, size_t bufsize);
int strap_time_parse_tz_offset(const char *str, int *offset_minutes);
int strap_time_format_iso8601(struct timeval t, int offset_minutes, char *buf, size_t bufsize);
int strap_time_parse_iso8601(const char *str, struct timeval *out, int *offset_minutes);

#endif /* STRAP_H */
