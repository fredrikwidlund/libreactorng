#ifndef REACTOR_VECTOR_H_INCLUDED
#define REACTOR_VECTOR_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <reactor.h>

typedef struct vector vector_t;
typedef void          vector_release_t(void *);
typedef int           vector_compare_t(const void *, const void *);

struct vector
{
  buffer_t buffer;
  size_t   object_size;
};

/* constructor/destructor */

void    vector_construct(vector_t *, size_t);
void    vector_destruct(vector_t *, vector_release_t);

/* capacity */

size_t  vector_size(const vector_t *);
size_t  vector_capacity(const vector_t *);
bool    vector_empty(const vector_t *);
void    vector_reserve(vector_t *, size_t);
void    vector_shrink_to_fit(vector_t *);

/* element access */

void   *vector_at(const vector_t *, size_t);
void   *vector_front(const vector_t *);
void   *vector_back(const vector_t *);
void   *vector_base(const vector_t *);

/* modifiers */

void    vector_insert(vector_t *, size_t, const void *);
void    vector_insert_range(vector_t *, size_t, const void *, const void *);
void    vector_insert_fill(vector_t *, size_t, size_t, const void *);
void    vector_erase(vector_t *, size_t, vector_release_t *);
void    vector_erase_range(vector_t *, size_t, size_t, vector_release_t *);
void    vector_push_back(vector_t *, const void *);
void    vector_pop_back(vector_t *, vector_release_t *);
void    vector_clear(vector_t *, vector_release_t *);

/* operations */

void    vector_sort(vector_t *, vector_compare_t *);

#endif /* REACTOR_VECTOR_H_INCLUDED */
