#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <reactor.h>

static void set(void *p1, const void *p2)
{
  maps_entry_t *a = p1;
  const maps_entry_t *b = p2;

  *a = b ? *b : (maps_entry_t) {0};
}

static int equal(const void *p1, const void *p2)
{
  const maps_entry_t *a = p1, *b = p2;

  return b ? strcmp(a->key, b->key) == 0 : a->key == NULL;
}

static size_t hash(const void *p)
{
  const maps_entry_t *a = p;

  return hash_data(data_string(a->key));
}

/* constructor/destructor */

void maps_construct(maps_t *maps)
{
  map_construct(&maps->map, sizeof(maps_entry_t), set);
}

void maps_destruct(maps_t *maps, maps_release_t *release)
{
  map_destruct(&maps->map, equal, (map_release_t *) release);
}

/* capacity */

size_t maps_size(const maps_t *maps)
{
  return map_size(&maps->map);
}

void maps_reserve(maps_t *maps, size_t size)
{
  map_reserve(&maps->map, size, hash, set, equal);
}

/* element access */

uintptr_t maps_at(const maps_t *maps, const char *key)
{
  return ((maps_entry_t *) map_at(&maps->map, (maps_entry_t[]) {{.key = key}}, hash, equal))->value;
}

/* modifiers */

void maps_insert(maps_t *maps, const char *key, uintptr_t value, maps_release_t *release)
{
  map_insert(&maps->map, (maps_entry_t[]) {{.key = key, .value = value}}, hash, set, equal, (map_release_t *) release);
}

void maps_erase(maps_t *maps, const char *key, maps_release_t *release)
{
  map_erase(&maps->map, (maps_entry_t[]) {{.key = key}}, hash, set, equal, (map_release_t *) release);
}

void maps_clear(maps_t *maps, maps_release_t *release)
{
  map_clear(&maps->map, set, equal, (map_release_t *) release);
}
