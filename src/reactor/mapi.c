#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <reactor.h>

static void set(void *p1, const void *p2)
{
  mapi_entry_t *a = p1;
  const mapi_entry_t *b = p2;

  *a = b ? *b : (mapi_entry_t) {0};
}

static int equal(const void *p1, const void *p2)
{
  const mapi_entry_t *a = p1, *b = p2;

  return b ? a->key == b->key : a->key == 0;
}

static size_t hash(const void *p)
{
  const mapi_entry_t *a = p;

  return hash_uint64(a->key);
}

/* constructor/destructor */

void mapi_construct(mapi_t *mapi)
{
  map_construct(&mapi->map, sizeof(mapi_entry_t), set);
}

void mapi_destruct(mapi_t *mapi, mapi_release_t *release)
{
  map_destruct(&mapi->map, equal, (map_release_t *) release);
}

/* capacity */

size_t mapi_size(const mapi_t *mapi)
{
  return map_size(&mapi->map);
}

void mapi_reserve(mapi_t *mapi, size_t size)
{
  map_reserve(&mapi->map, size, hash, set, equal);
}

/* element access */

uintptr_t mapi_at(const mapi_t *mapi, uintptr_t key)
{
  return ((mapi_entry_t *) map_at(&mapi->map, (mapi_entry_t[]) {{.key = key}}, hash, equal))->value;
}

/* modifiers */

void mapi_insert(mapi_t *mapi, uintptr_t key, uintptr_t value, mapi_release_t *release)
{
  map_insert(&mapi->map, (mapi_entry_t[]) {{.key = key, .value = value}}, hash, set, equal, (map_release_t *) release);
}

void mapi_erase(mapi_t *mapi, uintptr_t key, mapi_release_t *release)
{
  map_erase(&mapi->map, (mapi_entry_t[]) {{.key = key}}, hash, set, equal, (map_release_t *) release);
}

void mapi_clear(mapi_t *mapi, mapi_release_t *release)
{
  map_clear(&mapi->map, set, equal, (map_release_t *) release);
}
