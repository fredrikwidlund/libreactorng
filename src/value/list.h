#ifndef VALUE_LIST_H_INCLUDED
#define VALUE_LIST_H_INCLUDED

#include <stdlib.h>
#include <value.h>

#define list_foreach(l, o)         for ((o) = list_front(l); (o) != list_end(l); (o) = list_next(o))
#define list_foreach_reverse(l, o) for ((o) = list_back(l); (o) != list_end(l); (o) = list_previous(o))

typedef void             list_release_t(void *);
typedef int              list_compare_t(const void *, const void *);
typedef struct list_item list_item_t;
typedef struct list      list_t;

struct list
{
  list_item_t *next;
  list_item_t *previous;
};

struct list_item
{
  list_t       list;
  char         object[];
};

/* constructor/destructor */
void    list_construct(list_t *);
void    list_destruct(list_t *, list_release_t *);

/* iterators */
void   *list_next(const void *);
void   *list_previous(const void *);

/* capacity */
int     list_empty(const list_t *);

/* object access */
void   *list_front(const list_t *);
void   *list_back(const list_t *);
void   *list_end(const list_t *);

/* modifiers */
void   *list_push_front(list_t *, const void *, size_t);
void   *list_push_back(list_t *, const void *, size_t);
void   *list_insert(void *, const void *, size_t);
void    list_splice(void *, void *);
void    list_erase(void *, list_release_t *);
void    list_clear(list_t *, list_release_t *);

/* operations */
void   *list_find(const list_t *, list_compare_t *, const void *);

#endif /* VALUE_LIST_H_INCLUDED */
