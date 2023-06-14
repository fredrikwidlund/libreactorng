#ifndef VALUE_MAPI_H_INCLUDED
#define VALUE_MAPI_H_INCLUDED

#define mapi_foreach(m, e) \
  for ((e) = (m)->map.elements; (e) != ((mapi_entry *) (m)->map.elements) + (m)->map.elements_capacity; (e) ++) \
    if ((e)->key)

typedef struct mapi_entry mapi_entry_t;
typedef struct mapi       mapi_t;
typedef void              mapi_release_t(mapi_entry_t *);

struct mapi_entry
{
  uintptr_t key;
  uintptr_t value;
};

struct mapi
{
  map_t     map;
};

/* constructor/destructor */

void      mapi_construct(mapi_t *);
void      mapi_destruct(mapi_t *, mapi_release_t *);

/* capacity */

size_t    mapi_size(const mapi_t *);
void      mapi_reserve(mapi_t *, size_t);

/* element access */

uintptr_t mapi_at(const mapi_t *, uintptr_t);

/* modifiers */

void      mapi_insert(mapi_t *, uintptr_t, uintptr_t, mapi_release_t *);
void      mapi_erase(mapi_t *, uintptr_t, mapi_release_t *);
void      mapi_clear(mapi_t *, mapi_release_t *);

#endif /* VALUE_MAPI_H_INCLUDED */
