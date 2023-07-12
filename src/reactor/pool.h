#ifndef REACTOR_POOL_H
#define REACTOR_POOL_H

#include <stdlib.h>
#include <reactor.h>

typedef struct pool pool_t;

struct pool
{
  size_t   size;
  size_t   allocated;
  list_t   chunks;
  vector_t free;
};

void    pool_construct(pool_t *, size_t);
void    pool_destruct(pool_t *);
size_t  pool_size(pool_t *);
void   *pool_malloc(pool_t *);
void    pool_free(pool_t *, void *);

#endif /* REACTOR_POOL_H */
