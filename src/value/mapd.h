#ifndef VALUE_MAPD_H_INCLUDED
#define VALUE_MAPD_H_INCLUDED

#define mapd_foreach(m, e) \
  for ((e) = (m)->map.elements; (e) != ((mapd_entry_t *) (m)->map.elements) + (m)->map.elements_capacity; (e)++) \
    if (!data_nullp((e)->key))

typedef struct mapd_entry mapd_entry_t;
typedef struct mapd       mapd_t;
typedef void              mapd_release_t(mapd_entry_t *);

struct mapd_entry
{
  data_t    key;
  uintptr_t value;
};

struct mapd
{
  map_t     map;
};

/* constructor/destructor */

void      mapd_construct(mapd_t *);
void      mapd_destruct(mapd_t *, mapd_release_t *);

/* capacity */

size_t    mapd_size(const mapd_t *);
void      mapd_reserve(mapd_t *, size_t);

/* element access */

uintptr_t mapd_at(const mapd_t *, const data_t);

/* modifiers */

void      mapd_insert(mapd_t *, const data_t, uintptr_t, mapd_release_t *);
void      mapd_erase(mapd_t *, const data_t, mapd_release_t *);
void      mapd_clear(mapd_t *, mapd_release_t *);

#endif /* VALUE_MAPD_H_INCLUDED */
