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
  timeout_t       timeout;
  list_t          sessions;
};

struct server_session
{
  reactor_user_t  user;
  http_request_t  request;
  stream_t        stream;
  int             flags;
  bool           *abort;
  reactor_t       next;
};

void server_construct(server_t *, reactor_callback_t *, void *);
void server_destruct(server_t *);
void server_open(server_t *, const char *, int);
void server_open_socket(server_t *, int);
void server_close(server_t *);
void server_disconnect(server_session_t *);
void server_respond(server_session_t *, string_t, string_t, data_t, http_field_t *, size_t);
void server_ok(server_session_t *, string_t, data_t, http_field_t *, size_t);
void server_plain(server_session_t *, data_t, http_field_t *, size_t);

#endif /* REACTOR_SERVER_H_INCLUDED */
