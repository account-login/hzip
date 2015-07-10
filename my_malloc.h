/*
 * exit(1) after malloc() calloc() realloc() failed.
 * print extra info if NDEBUG is not defined
 */
#if !defined(_MY_MALLOC_H)
#define _MY_MALLOC_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void *__my_malloc_return;
#ifdef NDEBUG
#define malloc(size) (__my_malloc_return = malloc(size)) ? __my_malloc_return \
    : (perror("malloc()"), exit(1), NULL)
#define realloc(ptr, size) (__my_malloc_return = realloc(ptr, size)) ? __my_malloc_return \
    : (perror("realloc()"), exit(1), NULL)
#define calloc(num, size) (__my_malloc_return = calloc(num, size)) ? __my_malloc_return \
    : (perror("calloc()"), exit(1), NULL)
#else
#define malloc(size) (__my_malloc_return = malloc(size)) ? __my_malloc_return \
    : (fprintf(stderr, "malloc(%s) failed at file '%s', line %d. ", #size, __FILE__, __LINE__), \
       perror("reason"), exit(1), NULL)
#define realloc(ptr, size) (__my_malloc_return = realloc(ptr, size)) ? __my_malloc_return \
    : (fprintf(stderr, "realloc(%s, %s) failed at file '%s', line %d. ", #ptr, #size, __FILE__, __LINE__), \
       perror("reason"), exit(1), NULL)
#define calloc(num, size) (__my_malloc_return = calloc(num, size)) ? __my_malloc_return \
    : (fprintf(stderr, "calloc(%s, %s) failed at file '%s', line %d. ", #num, #size, __FILE__, __LINE__), \
       perror("reason"), exit(1), NULL)
#endif // NDEBUG


// duplicate memory
void *memdup(void *s, size_t len)
{
    void *d = malloc(len);
    if(d == NULL)
        return NULL;
    return memcpy(d, s, len);
}

#endif  // _MY_MALLOC_H
