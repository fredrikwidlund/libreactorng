#ifndef REACTOR_NOTIFY_H_INCLUDED
#define REACTOR_NOTIFY_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <sys/inotify.h>
#include <reactor.h>

enum
{
  NOTIFY_EVENT
};

typedef struct notify_event notify_event_t;
typedef struct notify_entry notify_entry_t;
typedef struct notify       notify_t;

struct notify_event
{
  int             id;
  char           *path;
  char           *name;
  uint32_t        mask;
};

struct notify_entry
{
  int             id;
  char           *path;
};

struct notify
{
  reactor_user_t  user;
  int             fd;
  reactor_id_t    read;
  reactor_id_t    next;
  list_t          entries;
  mapi_t          ids;
  maps_t          paths;
  bool           *abort;
  char            buffer[4096] __attribute__((aligned(__alignof__(struct inotify_event))));
};

void notify_construct(notify_t *, reactor_callback_t *, void *);
void notify_open(notify_t *);
int  notify_add(notify_t *, char *, uint32_t);
bool notify_remove_path(notify_t *, char *);
bool notify_remove_id(notify_t *, int);
void notify_close(notify_t *);
void notify_destruct(notify_t *);

#endif /* REACTOR_NOTIFY_H_INCLUDED */
