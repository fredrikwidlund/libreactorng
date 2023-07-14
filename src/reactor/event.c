#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/eventfd.h>
#include <reactor.h>

static void event_read(reactor_event_t *reactor_event)
{
  event_t *event = reactor_event->state;
  bool abort = false;

  event->read = 0;
  assert(reactor_event->data == sizeof event->counter);
  event->abort = &abort;
  reactor_call(&event->user, EVENT_SIGNAL, event->counter);
  if (abort)
    return;
  event->abort = NULL;
  event->read = reactor_read(event_read, event, event->fd, &event->counter, sizeof event->counter, 0);
}

void event_construct(event_t *event, reactor_callback_t *callback, void *state)
{
  *event = (event_t) {.user = reactor_user_define(callback, state), .fd = -1};
}

void event_open(event_t *event)
{
  event->fd = eventfd(0, event->counter);
  event->read = reactor_read(event_read, event, event->fd, &event->counter, sizeof event->counter, 0);
}

void event_signal(event_t *event)
{
  static const uint64_t signal = 1;

  (void) reactor_write(NULL, NULL, event->fd, &signal, sizeof signal, 0);
}

void event_signal_sync(event_t *event)
{
  ssize_t n;

  n = write(event->fd, (uint64_t[]) {1}, sizeof (uint64_t));
  assert(n == sizeof (uint64_t));
}

void event_close(event_t *event)
{
  if (event->fd == -1)
    return;

  if (event->abort)
    *event->abort = true;
  if (event->read)
    reactor_cancel(event->read, NULL, NULL);
  (void) reactor_close(NULL, NULL, event->fd);
  event->fd = -1;
}

void event_destruct(event_t *event)
{
  event_close(event);
}
