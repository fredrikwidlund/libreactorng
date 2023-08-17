#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <reactor.h>

/* internals */

static list_item_t *list_object_item(const void *object)
{
  return (list_item_t *) ((uintptr_t) object - offsetof(list_item_t, object));
}

static list_item_t *list_item_new(const void *object, size_t size)
{
  list_item_t *item;

  item = calloc(1, sizeof(list_item_t) + size);
  assert(item != NULL);
  if (object)
    memcpy(item->object, object, size);

  return item;
}

/* constructor/destructor */

void list_construct(list_t *list)
{
  list->next = (list_item_t *) list;
  list->previous = (list_item_t *) list;
}

void list_destruct(list_t *list, list_release_t *release)
{
  list_clear(list, release);
}

/* iterators */

void *list_next(const void *object)
{
  return list_object_item(object)->list.next->object;
}

void *list_previous(const void *object)
{
  return list_object_item(object)->list.previous->object;
}

/* capacity */

int list_empty(const list_t *list)
{
  return list->next == (list_item_t *) list;
}

/* element access */

void *list_front(const list_t *list)
{
  return list->next->object;
}

void *list_back(const list_t *list)
{
  return list->previous->object;
}

void *list_end(const list_t *list)
{
  return ((list_item_t *) list)->object;
}

/* modifiers */

void *list_push_front(list_t *list, const void *object, size_t size)
{
  return list_insert(list_front(list), object, size);
}

void *list_push_back(list_t *list, const void *object, size_t size)
{
  return list_insert(list_end(list), object, size);
}

void *list_insert(void *list_object, const void *object, size_t size)
{
  list_item_t *after, *item;

  after = list_object_item(list_object);
  item = list_item_new(object, size);
  item->list.previous = after->list.previous;
  item->list.next = after;
  item->list.previous->list.next = item;
  item->list.next->list.previous = item;
  return item->object;
}

void list_splice(void *object1, void *object2)
{
  list_item_t *to, *from;

  if (object1 == object2)
    return;
  to = list_object_item(object1);
  from = list_object_item(object2);
  from->list.previous->list.next = from->list.next;
  from->list.next->list.previous = from->list.previous;
  from->list.previous = to->list.previous;
  from->list.next = to;
  from->list.previous->list.next = from;
  from->list.next->list.previous = from;
}

void list_detach(void *object)
{
  list_item_t *item = list_object_item(object);

  item->list.previous->list.next = item->list.next;
  item->list.next->list.previous = item->list.previous;
  item->list.next = item;
  item->list.previous = item;
}

void list_erase(void *object, list_release_t *release)
{
  list_item_t *item = list_object_item(object);

  item->list.previous->list.next = item->list.next;
  item->list.next->list.previous = item->list.previous;
  if (release)
    release(object);
  free(item);
}

void list_clear(list_t *list, list_release_t *release)
{
  while (!list_empty(list))
    list_erase(list_front(list), release);
}

/* operations */

void *list_find(const list_t *list, list_compare_t *compare, const void *object)
{
  void *list_object;

  list_foreach(list, list_object) if (compare(object, list_object) == 0) return list_object;

  return NULL;
}
