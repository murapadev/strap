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

/* Trim (inplace or return new) */
char *strtrim(const char *s);  /* returns new malloc() without spaces at start/end */
void strtrim_inplace(char *s); /* modifies buffer in-place */

/* Time utilities (struct timeval) */
struct timeval timeval_add(struct timeval a, struct timeval b);
struct timeval timeval_sub(struct timeval a, struct timeval b);
double timeval_to_seconds(struct timeval t);

#endif /* STRAP_H */
