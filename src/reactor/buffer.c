#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <reactor.h>

static size_t buffer_roundup(size_t size)
{
  size--;
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  size |= size >> 32;
  size++;
  return size;
}

/* constructor/destructor */

void buffer_construct(buffer_t *buffer)
{
  *buffer = (buffer_t) {0};
}

void buffer_destruct(buffer_t *buffer)
{
  data_release(buffer->data);
}

void *buffer_deconstruct(buffer_t *buffer)
{
  void *base = buffer_base(buffer);

  buffer_construct(buffer);
  return base;
}

/* capacity */

size_t buffer_size(const buffer_t *buffer)
{
  return data_size(buffer->data);
}

size_t buffer_capacity(const buffer_t *buffer)
{
  return buffer->capacity;
}

bool buffer_empty(const buffer_t *buffer)
{
  return buffer_size(buffer) == 0;
}

void buffer_reserve(buffer_t *buffer, size_t capacity)
{
  if (capacity > buffer->capacity)
  {
    capacity = buffer_roundup(capacity);
    buffer->data = data(realloc(buffer_base(buffer), capacity), buffer_size(buffer));
    buffer->capacity = capacity;
  }
}

void buffer_resize(buffer_t *buffer, size_t size)
{
  if (size > buffer->capacity)
    buffer_reserve(buffer, size);
  buffer->data = data_select(buffer->data, size);
}

void buffer_compact(buffer_t *buffer)
{
  size_t size;

  size = buffer_size(buffer);
  if (buffer->capacity > size)
  {
    buffer->data = data(realloc(buffer_base(buffer), size), size);
    buffer->capacity = size;
  }
}

data_t buffer_allocate(buffer_t *buffer, size_t data_size)
{
  size_t size;

  size = buffer_size(buffer);
  buffer_resize(buffer, size + data_size);
  return data_offset(buffer->data, size);
}

/* modifiers */

void buffer_insert(buffer_t *restrict buffer, size_t pos, const data_t data)
{
  buffer_reserve(buffer, buffer_size(buffer) + data_size(data));
  if (pos < buffer_size(buffer))
    memmove(buffer_at(buffer, pos + data_size(data)), buffer_at(buffer, pos), buffer_size(buffer) - pos);
  memcpy(buffer_at(buffer, pos), data_base(data), data_size(data));
  buffer_resize(buffer, buffer_size(buffer) + data_size(data));
}

void buffer_insert_fill(buffer_t *restrict buffer, size_t pos, size_t count, const data_t data)
{
  size_t i;

  buffer_reserve(buffer, buffer_size(buffer) + (count * data_size(data)));
  if (pos < buffer_size(buffer))
    memmove(buffer_at(buffer, pos + (count * data_size(data))), buffer_at(buffer, pos), buffer_size(buffer) - pos);
  for (i = 0; i < count; i++)
    memcpy(buffer_at(buffer, pos + (i * data_size(data))), data_base(data), data_size(data));
  buffer_resize(buffer, buffer_size(buffer) + (count * data_size(data)));
}

void buffer_prepend(buffer_t *restrict buffer, const data_t data)
{
  buffer_insert(buffer, 0, data);
}

void buffer_append(buffer_t *restrict buffer, const data_t data)
{
  buffer_insert(buffer, buffer_size(buffer), data);
}

void buffer_erase(buffer_t *restrict buffer, size_t position, size_t size)
{
  memmove(buffer_at(buffer, position), buffer_at(buffer, position + size), buffer_size(buffer) - position - size);
  buffer_resize(buffer, buffer_size(buffer) - size);
}

void buffer_clear(buffer_t *restrict buffer)
{
  buffer_resize(buffer, 0);
}

void buffer_read(buffer_t *restrict buffer, FILE *restrict file)
{
  size_t n;

  do
  {
    n = fread(data_base(buffer_allocate(buffer, 4096)), 1, 4096, file);
    buffer_resize(buffer, buffer_size(buffer) - 4096 + n);
  } while (n == 4096);
}

bool buffer_load(buffer_t *restrict buffer, const char *path)
{
  FILE *file;

  file = fopen(path, "r");
  if (!file)
    return false;
  buffer_clear(buffer);
  buffer_read(buffer, file);
  buffer_compact(buffer);
  fclose(file);
  return true;
}

bool buffer_loadz(buffer_t *restrict buffer, const char *path)
{
  FILE *file;

  file = fopen(path, "r");
  if (!file)
    return false;
  buffer_clear(buffer);
  buffer_read(buffer, file);
  buffer_append(buffer, data("", 1));
  buffer_compact(buffer);
  buffer_resize(buffer, buffer_size(buffer) - 1);
  fclose(file);
  return true;
}

void buffer_switch(buffer_t *restrict b1, buffer_t *restrict b2)
{
  buffer_t tmp;

  tmp = *b1;
  *b1 = *b2;
  *b2 = tmp;
}

/* element access */

data_t buffer_data(const buffer_t *buffer)
{
  return buffer->data;
}

void *buffer_base(const buffer_t *buffer)
{
  return data_base(buffer->data);
}

void *buffer_at(const buffer_t *buffer, size_t pos)
{
  return (char *) buffer_base(buffer) + pos;
}

void *buffer_end(const buffer_t *buffer)
{
  return buffer_at(buffer, buffer_size(buffer));
}

/* operations */

void buffer_write(const buffer_t *restrict buffer, FILE *restrict file)
{
  (void) fwrite(buffer_base(buffer), buffer_size(buffer), 1, file);
}

bool buffer_save(const buffer_t *restrict buffer, const char *path)
{
  FILE *file;

  file = fopen(path, "w");
  if (!file)
    return false;
  buffer_write(buffer, file);
  fclose(file);
  return true;
}
