#ifndef REACTOR_TIMEOUT_H_INCLUDED
#define REACTOR_TIMEOUT_H_INCLUDED

#include <linux/time_types.h>
#include <reactor.h>

enum
{
  TIMEOUT_ALERT
};

typedef struct timeout timeout_t;

struct timeout
{
  reactor_user_t  user;
  reactor_time_t  time;
  reactor_time_t  delay;
  reactor_t       timeout;
  struct timespec tv;
};

void timeout_construct(timeout_t *, reactor_callback_t *, void *);
void timeout_destruct(timeout_t *);
void timeout_set(timeout_t *, reactor_time_t, reactor_time_t);
void timeout_clear(timeout_t *);

#endif /* REACTOR_TIMEOUT_H_INCLUDED */
