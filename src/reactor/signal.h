#ifndef REACTOR_SIGNAL_H_INCLUDED
#define REACTOR_SIGNAL_H_INCLUDED

#include <stdbool.h>
#include <reactor.h>

enum
{
  SIGNAL_RECEIVE
};

typedef struct signal signal_t;

struct signal
{
  reactor_user_t  user;
  reactor_t       read;
  uint64_t        counter;
  int             fd;
  bool           *abort;
};

void signal_construct(signal_t *, reactor_callback_t *, void *);
void signal_open(signal_t *);
void signal_send(signal_t *);
void signal_send_sync(signal_t *);
void signal_close(signal_t *);
void signal_destruct(signal_t *);

#endif /* REACTOR_SIGNAL_H_INCLUDED */

