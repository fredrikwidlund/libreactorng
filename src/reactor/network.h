#ifndef REACTOR_NETWORK_H_INCLUDED
#define REACTOR_NETWORK_H_INCLUDED

#include <reactor.h>
#include <sys/socket.h>

enum
{
  NETWORK_ERROR,
  NETWORK_RESOLVE
};

typedef reactor_id_t network_t;

void      network_construct(void);
void      network_destruct(void);
void      network_cancel(network_t);
void      network_expire(uint64_t);

network_t network_resolve(reactor_callback_t *, void *, const char *);

#endif /* REACTOR_NETWORK_H_INCLUDED */
