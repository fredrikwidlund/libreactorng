#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <value.h>

static void set(void *p1, const void *p2)
{
  mapd_entry_t *a = p1;
  const mapd_entry_t *b = p2;

  *a = b ? *b : (mapd_entry_t) {0};
}

static int equal(const void *p1, const void *p2)
{
  const mapd_entry_t *a = p1, *b = p2;

  return b ? data_equal(a->key, b->key) : data_base(a->key) == NULL;
}

static size_t hash(const void *p)
{
  const mapd_entry_t *a = p;

  return hash_data(a->key);
}

/* constructor/destructor */

void mapd_construct(mapd_t *mapd)
{
  map_construct(&mapd->map, sizeof(mapd_entry_t), set);
}

void mapd_destruct(mapd_t *mapd, mapd_release_t *release)
{
  map_destruct(&mapd->map, equal, (map_release_t *) release);
}

/* capacity */

size_t mapd_size(const mapd_t *mapd)
{
  return map_size(&mapd->map);
}

void mapd_reserve(mapd_t *mapd, size_t size)
{
  map_reserve(&mapd->map, size, hash, set, equal);
}

/* element access */

uintptr_t mapd_at(const mapd_t *mapd, const data_t key)
{
  return ((mapd_entry_t *) map_at(&mapd->map, (mapd_entry_t[]) {{.key = key}}, hash, equal))->value;
}

/* modifiers */

void mapd_insert(mapd_t *mapd, const data_t key, uintptr_t value, mapd_release_t *release)
{
  map_insert(&mapd->map, (mapd_entry_t[]) {{.key = key, .value = value}}, hash, set, equal, (map_release_t *) release);
}

void mapd_erase(mapd_t *mapd, const data_t key, mapd_release_t *release)
{
  map_erase(&mapd->map, (mapd_entry_t[]) {{.key = key}}, hash, set, equal, (map_release_t *) release);
}

void mapd_clear(mapd_t *mapd, mapd_release_t *release)
{
  map_clear(&mapd->map, set, equal, (map_release_t *) release);
}
