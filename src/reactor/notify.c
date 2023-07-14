#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <sys/inotify.h>
#include <reactor.h>

static void notify_read(reactor_event_t *);

static void notify_update(notify_t *notify)
{
  if (notify->read)
  {
    if (list_empty(&notify->entries))
    {
      reactor_cancel(notify->read, NULL, NULL);
      notify->read = 0;
    }
  }
  else
  {
    if (!list_empty(&notify->entries))
    {
      notify->read = reactor_read(notify_read, notify, notify->fd, notify->buffer, sizeof notify->buffer, 0);
    }
  }
}

static void notify_next(reactor_event_t *event)
{
  notify_t *notify = event->state;

  notify->next = 0;
  notify_update(notify);
}

static void notify_read(reactor_event_t *event)
{
  notify_t *notify = event->state;
  struct inotify_event *i;
  notify_entry_t *e;
  char *p;
  int result = event->data;
  bool abort = false;

  notify->read = 0;
  assert(result >= 0);
  notify->abort = &abort;
  for (p = notify->buffer; p < notify->buffer + result; p += sizeof(struct inotify_event) + i->len)
    {
      i = (struct inotify_event *) p;
      e = (notify_entry_t *) mapi_at(&notify->ids, i->wd);
      if (e)
        reactor_call(&notify->user, NOTIFY_EVENT, (uint64_t) (notify_event_t[])
                     {{.mask = i->mask, .id = e->id, .name = i->len ? i->name : "", .path = e->path}});
      if (abort)
        return;
    }
  notify->abort = NULL;
  notify_update(notify);
}

static void notify_remove_entry(notify_t *notify, notify_entry_t *entry)
{
  int e;

  e = inotify_rm_watch(notify->fd, entry->id);
  assert(e == 0);
  mapi_erase(&notify->ids, entry->id, NULL);
  maps_erase(&notify->paths, entry->path, NULL);
  free(entry->path);
  list_erase(entry, NULL);
  if (!notify->next)
    notify->next = reactor_next(notify_next, notify);
}

void notify_construct(notify_t *notify, reactor_callback_t *callback, void *state)
{
  *notify = (notify_t) {.user = reactor_user_define(callback, state), .fd = -1};
  list_construct(&notify->entries);
  mapi_construct(&notify->ids);
  maps_construct(&notify->paths);
}

int notify_add(notify_t *notify, char *path, uint32_t mask)
{
  notify_entry_t *entry;
  int id;

  entry = (notify_entry_t *) maps_at(&notify->paths, path);
  if (entry)
    return entry->id;

  id = inotify_add_watch(notify->fd, path, mask);
  if (id == -1)
    return id;

  entry = list_push_back(&notify->entries, NULL, sizeof *entry);
  entry->id = id;
  entry->path = strdup(path);

  mapi_insert(&notify->ids, entry->id, (uint64_t) entry, NULL);
  maps_insert(&notify->paths, entry->path, (uint64_t) entry, NULL);
  notify_update(notify);
  return id;
}

bool notify_remove_path(notify_t *notify, char *path)
{
  notify_entry_t *e;

  e = (notify_entry_t *) maps_at(&notify->paths, path);
  if (!e)
    return false;
  notify_remove_entry(notify, e);
  return true;
}

bool notify_remove_id(notify_t *notify, int id)
{
  notify_entry_t *e;

  e = (notify_entry_t *) mapi_at(&notify->ids, id);
  if (!e)
    return false;
  notify_remove_entry(notify, e);
  return true;
}

void notify_open(notify_t *notify)
{
  assert(notify->fd == -1);
  notify->fd = inotify_init1(IN_CLOEXEC);
  assert(notify->fd >= 0);
}

void notify_close(notify_t *notify)
{
  if (notify->fd == -1)
    return;

  if (notify->abort)
    *notify->abort = true;

  if (notify->next)
  {
    reactor_cancel(notify->next, NULL, NULL);
    notify->next = 0;
  }

  while (!list_empty(&notify->entries))
    notify_remove_entry(notify, list_front(&notify->entries));
  (void) reactor_close(NULL, NULL, notify->fd);
  notify->fd = -1;
}

void notify_destruct(notify_t *notify)
{
  notify_close(notify);
  list_destruct(&notify->entries, NULL);
  mapi_destruct(&notify->ids, NULL);
  maps_destruct(&notify->paths, NULL);
}
