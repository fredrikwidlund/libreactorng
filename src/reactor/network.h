#ifndef REACTOR_NETWORK_H_INCLUDED
#define REACTOR_NETWORK_H_INCLUDED

#include <reactor.h>
#include <sys/socket.h>

enum
{
  NETWORK_ERROR,
  NETWORK_RESOLVE,
  NETWORK_ACCEPT,
  NETWORK_ACCEPT_BIND,
  NETWORK_CONNECT
};

enum
{
  NETWORK_REUSEADDR = 0x01,
  NETWORK_REUSEPORT = 0x02
};

typedef reactor_t network_t;

void      network_construct(void);
void      network_destruct(void);
void      network_cancel(network_t);
void      network_expire(uint64_t);

network_t network_resolve(reactor_callback_t *, void *, const char *);

network_t network_accept(reactor_callback_t *, void *, const char *, int, int);
network_t network_accept_address(reactor_callback_t *, void *, struct sockaddr *, socklen_t, int);
network_t network_accept_socket(reactor_callback_t *, void *, int);

network_t network_connect(reactor_callback_t *, void *, const char *, int);
network_t network_connect_address(reactor_callback_t *, void *, struct sockaddr *, socklen_t);

#endif /* REACTOR_NETWORK_H_INCLUDED */
