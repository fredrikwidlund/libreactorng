#ifndef REACTOR_MAP_H_INCLUDED
#define REACTOR_MAP_H_INCLUDED

#ifndef MAP_ELEMENTS_CAPACITY_MIN
#define MAP_ELEMENTS_CAPACITY_MIN 8
#endif /* MAP_ELEMENTS_CAPACITY_MIN */

typedef size_t     map_hash_t(const void *);
typedef int        map_equal_t(const void *, const void *);
typedef void       map_set_t(void *, const void *);
typedef void       map_release_t(void *);
typedef struct map map_t;

struct map
{
  void   *elements;
  size_t  elements_count;
  size_t  elements_capacity;
  size_t  element_size;
};

/* constructor/destructor */

void    map_construct(map_t *, size_t, map_set_t *);
void    map_destruct(map_t *, map_equal_t *, map_release_t *);

/* capacity */

size_t  map_size(const map_t *);
void    map_reserve(map_t *, size_t, map_hash_t *, map_set_t *, map_equal_t *);

/* element access */

void   *map_at(const map_t *, const void *, map_hash_t *, map_equal_t *);

/* modifiers */

void    map_insert(map_t *, void *, map_hash_t *, map_set_t *, map_equal_t *, map_release_t *);
void    map_erase(map_t *, const void *,  map_hash_t *, map_set_t *, map_equal_t *, map_release_t *);
void    map_clear(map_t *, map_set_t *, map_equal_t *, map_release_t *);

#endif /* REACTOR_MAP_H_INCLUDED */
