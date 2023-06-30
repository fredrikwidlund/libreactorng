#ifndef REACTOR_MAPS_H_INCLUDED
#define REACTOR_MAPS_H_INCLUDED

#define maps_foreach(m, e) \
  for ((e) = (m)->map.elements; (e) != ((maps_entry *) (m)->map.elements) + (m)->map.elements_capacity; (e) ++) \
    if ((e)->key)

typedef struct maps_entry maps_entry_t;
typedef struct maps       maps_t;
typedef void              maps_release_t(maps_entry_t *);

struct maps_entry
{
  const char *key;
  uintptr_t   value;
};

struct maps
{
  map_t       map;
};

/* constructor/destructor */

void      maps_construct(maps_t *);
void      maps_destruct(maps_t *, maps_release_t *);

/* capacity */

size_t    maps_size(const maps_t *);
void      maps_reserve(maps_t *, size_t);

/* element access */

uintptr_t maps_at(const maps_t *, const char *);

/* modifiers */

void      maps_insert(maps_t *, const char *, uintptr_t, maps_release_t *);
void      maps_erase(maps_t *, const char *, maps_release_t *);
void      maps_clear(maps_t *, maps_release_t *);

#endif /* REACTOR_MAPS_H_INCLUDED */
