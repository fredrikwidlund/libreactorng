#ifndef REACTOR_BUFFER_H_INCLUDED
#define REACTOR_BUFFER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/uio.h>
#include <reactor.h>

typedef struct buffer buffer_t;

struct buffer
{
  data_t data;
  size_t capacity;
};

/* constructor/destructor */

void    buffer_construct(buffer_t *);
void    buffer_destruct(buffer_t *);
void   *buffer_deconstruct(buffer_t *);

/* capacity */

size_t  buffer_size(const buffer_t *);
size_t  buffer_capacity(const buffer_t *);
bool    buffer_empty(const buffer_t *);
void    buffer_reserve(buffer_t *, size_t);
void    buffer_resize(buffer_t *, size_t);
void    buffer_compact(buffer_t *);
data_t  buffer_allocate(buffer_t *, size_t);

/* modifiers */

void    buffer_insert(buffer_t *restrict, size_t, const data_t);
void    buffer_insert_fill(buffer_t *restrict, size_t, size_t, const data_t);
void    buffer_prepend(buffer_t *restrict, const data_t);
void    buffer_append(buffer_t *restrict, const data_t);
void    buffer_erase(buffer_t *restrict, size_t, size_t);
void    buffer_clear(buffer_t *);
void    buffer_read(buffer_t *restrict, FILE *restrict);
bool    buffer_load(buffer_t *, const char *);
bool    buffer_loadz(buffer_t *restrict, const char *);
void    buffer_switch(buffer_t *restrict, buffer_t *restrict);

/* element access */

data_t  buffer_data(const buffer_t *);
void   *buffer_base(const buffer_t *);
void   *buffer_at(const buffer_t *, size_t);
void   *buffer_end(const buffer_t *);

/* operations */

void    buffer_write(const buffer_t *restrict, FILE *restrict);
bool    buffer_save(const buffer_t *restrict, const char *);

#endif /* REACTOR_BUFFER_H_INCLUDED */
