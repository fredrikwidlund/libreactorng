#include <stdio.h>
#include <err.h>
#include <sys/inotify.h>

#include <reactor.h>

static void callback(reactor_event_t *event)
{
  notify_t *notify = event->state;
  notify_event_t *e = (notify_event_t *) event->data;

  (void) fprintf(stdout, "%d:%04x:%s:%s\n", e->id, e->mask, e->path, e->name);
  notify_remove_path(notify, e->path);
}

int main(int argc, char **argv)
{
  notify_t notify;
  int i, e;

  reactor_construct();
  notify_construct(&notify, callback, &notify);
  notify_open(&notify);
  for (i = 1; i < argc; i++)
  {
    e = notify_add(&notify, argv[i], IN_ALL_EVENTS);
    if (e == -1)
      err(1, "notify_add");
  }
  reactor_loop();
  notify_destruct(&notify);
  reactor_destruct();
}
