#ifndef REACTOR_SERVER_H_INCLUDED
#define REACTOR_SERVER_H_INCLUDED

enum
{
  SERVER_ERROR,
  SERVER_REQUEST
};

enum
{
  SERVER_SESSION_READY      = 0x01,
  SERVER_SESSION_PROCESSING = 0x02
};

typedef struct server         server_t;
typedef struct server_session server_session_t;

struct server
{
  reactor_user_t  user;
  network_t       accept;
  list_t          sessions;
};

struct server_session
{
  http_request_t  request;
  server_t       *server;
  stream_t        stream;
  int             flags;
  bool           *abort;

  size_t          ref;
  reactor_t       next;
};

void server_construct(server_t *, reactor_callback_t *, void *);
void server_destruct(server_t *);
void server_open(server_t *, const char *, int);
void server_close(server_t *);
void server_hello(server_session_t *);

#endif /* REACTOR_SERVER_H_INCLUDED */
