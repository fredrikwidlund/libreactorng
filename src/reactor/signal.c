#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <assert.h>
#include <sys/eventfd.h>
#include <reactor.h>

static void signal_read(reactor_event_t *reactor_event)
{
  signal_t *signal = reactor_event->state;
  bool abort = false;

  signal->read = 0;
  assert(reactor_event->data == sizeof signal->counter);
  signal->abort = &abort;
  reactor_call(&signal->user, SIGNAL_RECEIVE, signal->counter);
  if (abort)
    return;
  signal->abort = NULL;
  signal->read = reactor_read(signal_read, signal, signal->fd, &signal->counter, sizeof signal->counter, 0);
}

void signal_construct(signal_t *signal, reactor_callback_t *callback, void *state)
{
  *signal = (signal_t) {.user = reactor_user_define(callback, state), .fd = -1};
}

void signal_open(signal_t *signal)
{
  signal->fd = eventfd(0, signal->counter);
  signal->read = reactor_read(signal_read, signal, signal->fd, &signal->counter, sizeof signal->counter, 0);
}

void signal_send(signal_t *signal)
{
  static const uint64_t flag = 1;

  (void) reactor_write(NULL, NULL, signal->fd, &flag, sizeof flag, 0);
}

void signal_send_sync(signal_t *signal)
{
  ssize_t n;

  n = write(signal->fd, (uint64_t[]) {1}, sizeof (uint64_t));
  assert(n == sizeof (uint64_t));
}

void signal_close(signal_t *signal)
{
  if (signal->fd == -1)
    return;

  if (signal->abort)
    *signal->abort = true;
  if (signal->read)
    reactor_cancel(signal->read, NULL, NULL);
  (void) reactor_close(NULL, NULL, signal->fd);
  signal->fd = -1;
}

void signal_destruct(signal_t *signal)
{
  signal_close(signal);
}
