#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <value.h>

/* constructor/destructor */

void vector_construct(vector_t *vector, size_t object_size)
{
  buffer_construct(&vector->buffer);
  vector->object_size = object_size;
}

void vector_destruct(vector_t *vector, vector_release_t *release)
{
  vector_clear(vector, release);
  buffer_destruct(&vector->buffer);
}

/* capacity */

size_t vector_size(const vector_t *vector)
{
  return buffer_size(&vector->buffer) / vector->object_size;
}

size_t vector_capacity(const vector_t *vector)
{
  return buffer_capacity(&vector->buffer) / vector->object_size;
}

bool vector_empty(const vector_t *vector)
{
  return vector_size(vector) == 0;
}

void vector_reserve(vector_t *vector, size_t capacity)
{
  buffer_reserve(&vector->buffer, capacity * vector->object_size);
}

void vector_shrink_to_fit(vector_t *vector)
{
  buffer_compact(&vector->buffer);
}

/* element access */

void *vector_at(const vector_t *vector, size_t position)
{
  return buffer_at(&vector->buffer, position * vector->object_size);
}

void *vector_front(const vector_t *vector)
{
  return vector_base(vector);
}

void *vector_back(const vector_t *vector)
{
  return buffer_at(&vector->buffer, buffer_size(&vector->buffer) - vector->object_size);
}

void *vector_base(const vector_t *vector)
{
  return buffer_base(&vector->buffer);
}

/* modifiers */

void vector_insert(vector_t *vector, size_t position, const void *object)
{
  buffer_insert(&vector->buffer, position * vector->object_size, data_define(object, vector->object_size));
}

void vector_insert_range(vector_t *vector, size_t position, const void *first, const void *last)
{
  buffer_insert(&vector->buffer, position * vector->object_size, data_define(first, (char *) last - (char *) first));
}

void vector_insert_fill(vector_t *vector, size_t position, size_t count, const void *object)
{
  buffer_insert_fill(&vector->buffer, position * vector->object_size, count, data_define(object, vector->object_size));
}

void vector_erase(vector_t *vector, size_t position, vector_release_t *release)
{
  if (release)
    release(vector_at(vector, position));

  buffer_erase(&vector->buffer, position * vector->object_size, vector->object_size);
}

void vector_erase_range(vector_t *vector, size_t first, size_t last, vector_release_t *release)
{
  size_t i;

  if (release)
    for (i = first; i < last; i++)
      release(vector_at(vector, i));

  buffer_erase(&vector->buffer, first * vector->object_size, (last - first) * vector->object_size);
}

void vector_clear(vector_t *vector, vector_release_t *release)
{
  vector_erase_range(vector, 0, vector_size(vector), release);
  buffer_clear(&vector->buffer);
}

void vector_push_back(vector_t *vector, const void *object)
{
  buffer_insert(&vector->buffer, buffer_size(&vector->buffer), data_define(object, vector->object_size));
}

void vector_pop_back(vector_t *vector, vector_release_t *release)
{
  vector_erase(vector, vector_size(vector) - 1, release);
}

/* operations */

void vector_sort(vector_t *vector, vector_compare_t *compare)
{
  qsort(vector_base(vector), vector_size(vector), vector->object_size, compare);
}
