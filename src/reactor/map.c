#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <reactor.h>

static size_t map_roundup(size_t s)
{
  s--;
  s |= s >> 1;
  s |= s >> 2;
  s |= s >> 4;
  s |= s >> 8;
  s |= s >> 16;
  s |= s >> 32;
  s++;

  return s;
}

static void *map_element(const map_t *map, size_t position)
{
  return (char *) map->elements + (position * map->element_size);
}

static void map_release_all(map_t *map, map_equal_t *equal, map_release_t *release)
{
  size_t i;

  if (release)
    for (i = 0; i < map->elements_capacity; i++)
      if (!equal(map_element(map, i), NULL))
        release(map_element(map, i));
}

static void map_rehash(map_t *map, size_t size, map_hash_t *hash, map_set_t *set, map_equal_t *equal)
{
  map_t new;
  size_t i;

  size = map_roundup(size);
  new = *map;
  new.elements_count = 0;
  new.elements_capacity = size;
  new.elements = malloc(new.elements_capacity *new.element_size);

  for (i = 0; i < new.elements_capacity; i++)
    set(map_element(&new, i), NULL);

  if (map->elements)
  {
    for (i = 0; i < map->elements_capacity; i++)
      if (!equal(map_element(map, i), NULL))
        map_insert(&new, map_element(map, i), hash, set, equal, NULL);
    free(map->elements);
  }

  *map = new;
}

/* constructor/destructor */

void map_construct(map_t *map, size_t element_size, map_set_t *set)
{
  map->elements = NULL;
  map->elements_count = 0;
  map->elements_capacity = 0;
  map->element_size = element_size;
  map_rehash(map, MAP_ELEMENTS_CAPACITY_MIN, NULL, set, NULL);
}

void map_destruct(map_t *map, map_equal_t *equal, map_release_t *release)
{
  map_release_all(map, equal, release);
  free(map->elements);
}

/* capacity */

size_t map_size(const map_t *map)
{
  return map->elements_count;
}

void map_reserve(map_t *map, size_t size, map_hash_t *hash, map_set_t *set, map_equal_t *equal)
{
  size *= 2;
  if (size > map->elements_capacity)
    map_rehash(map, size, hash, set, equal);
}

/* element access */

void *map_at(const map_t *map, const void *element, map_hash_t *hash, map_equal_t *equal)
{
  size_t i;
  void *test;

  i = hash(element);
  while (1)
  {
    i &= map->elements_capacity - 1;
    test = map_element(map, i);
    if (equal(test, NULL) || equal(test, element))
      return test;
    i++;
  }
}

/* modifiers */

void map_insert(map_t *map, void *element, map_hash_t *hash, map_set_t *set, map_equal_t *equal, map_release_t *release)
{
  void *test;

  map_reserve(map, map->elements_count + 1, hash, set, equal);
  test = map_at(map, element, hash, equal);
  if (equal(test, NULL))
  {
    set(test, element);
    map->elements_count++;
  }
  else if (release)
    release(element);
}

void map_erase(map_t *map, const void *element, map_hash_t *hash, map_set_t *set, map_equal_t *equal,
               map_release_t *release)
{
  void *test;
  size_t i, j, k;

  i = hash(element);
  while (1)
  {
    i &= map->elements_capacity - 1;
    test = map_element(map, i);
    if (equal(test, NULL))
      return;
    if (equal(test, element))
      break;
    i++;
  }

  if (release)
    release(test);
  map->elements_count--;

  j = i;
  while (1)
  {
    j = (j + 1) & (map->elements_capacity - 1);
    if (equal(map_element(map, j), NULL))
      break;

    k = hash(map_element(map, j)) & (map->elements_capacity - 1);
    if ((i < j && (k <= i || k > j)) || (i > j && (k <= i && k > j)))
    {
      set(map_element(map, i), map_element(map, j));
      i = j;
    }
  }

  set(map_element(map, i), NULL);
}

void map_clear(map_t *map, map_set_t *set, map_equal_t *equal, map_release_t *release)
{
  map_release_all(map, equal, release);
  free(map->elements);
  map->elements = NULL;
  map->elements_count = 0;
  map->elements_capacity = 0;
  map_rehash(map, MAP_ELEMENTS_CAPACITY_MIN, NULL, set, NULL);
}
