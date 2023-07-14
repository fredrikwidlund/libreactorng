#ifndef REACTOR_RESOLVER_H
#define REACTOR_RESOLVER_H

#include <netdb.h>
#include <reactor.h>

enum
{
  RESOLVER_OK,
  RESOLVER_ERROR
};

typedef struct resolver resolver_t;

struct resolver
{
  reactor_user_t   user;
  struct addrinfo  hints;
  char            *host;
  char            *port;
  reactor_id_t     call;
  int              code;
  struct addrinfo *result;
};

void resolver_construct(resolver_t *, reactor_callback_t *, void *);
void resolver_destruct(resolver_t *);
void resolver_lookup(resolver_t *, int, int, int, int, char *, char *);

#endif /* REACTOR_RESOLVER_H */
