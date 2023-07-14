#ifndef REACTOR_EVENT_H_INCLUDED
#define REACTOR_EVENT_H_INCLUDED

#include <stdbool.h>
#include <reactor.h>

enum
{
  EVENT_SIGNAL
};

typedef struct event event_t;

struct event
{
  reactor_user_t  user;
  reactor_id_t    read;
  uint64_t        counter;
  int             fd;
  bool           *abort;
};

void event_construct(event_t *, reactor_callback_t *, void *);
void event_open(event_t *);
void event_signal(event_t *);
void event_signal_sync(event_t *);
void event_close(event_t *);
void event_destruct(event_t *);

#endif /* REACTOR_EVENT_H_INCLUDED */

