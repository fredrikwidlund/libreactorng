#include <linux/time_types.h>
#include <linux/io_uring.h>
#include <reactor.h>

static void timeout_timeout(reactor_event_t *);

static void timeout_submit(timeout_t *timeout)
{
  timeout->tv.tv_sec = timeout->time / 1000000000ULL;
  timeout->tv.tv_nsec = timeout->time %  1000000000ULL;
  timeout->timeout = reactor_timeout(timeout_timeout, timeout, &timeout->tv, 0, IORING_TIMEOUT_ABS | IORING_TIMEOUT_REALTIME);
}

static void timeout_timeout(reactor_event_t *event)
{
  timeout_t *timeout = event->state;
  reactor_time_t time;

  time = timeout->time;
  timeout->timeout = 0;
  if (timeout->delay)
  {
    timeout->time += timeout->delay;
    timeout_submit(timeout);
  }
  reactor_call(&timeout->user, TIMEOUT_ALERT, (uintptr_t) &time);
}

void timeout_construct(timeout_t *timeout, reactor_callback_t *callback, void *state)
{
  *timeout = (timeout_t) {.user = reactor_user_define(callback, state)};
}

void timeout_destruct(timeout_t *timeout)
{
  timeout_clear(timeout);
}

void timeout_set(timeout_t *timeout, reactor_time_t time, reactor_time_t delay)
{
  timeout_clear(timeout);
  timeout->time = time;
  timeout->delay = delay;
  timeout_submit(timeout);
}

void timeout_clear(timeout_t *timeout)
{
  if (timeout->timeout)
  {
    reactor_cancel(timeout->timeout, NULL, NULL);
    timeout->timeout = 0;
  }
}
