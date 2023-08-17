#include <string.h>
#include <reactor.h>

static __thread char server_date[30] = {0};

static void server_date_update(void)
{
  time_t t;
  struct tm tm;
  static const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
  static const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  t = (reactor_now() + 10000000) / 1000000000;
  (void) gmtime_r(&t, &tm);
  (void) strftime(server_date, sizeof server_date, "---, %d --- %Y %H:%M:%S GMT", &tm);
  memcpy(server_date, days[tm.tm_wday], 3);
  memcpy(server_date + 8, months[tm.tm_mon], 3);
}

static void server_session_close(server_session_t *session)
{
  stream_close(&session->stream);
  if (session->abort)
    *session->abort = true;
  if (session->next)
  {
    reactor_cancel(session->next, NULL, NULL);
    session->next = 0;
  }
  if (session->flags & SERVER_SESSION_READY)
  {
    stream_destruct(&session->stream);
    list_erase(session, NULL);
  }
}

static void server_session_read(server_session_t *session)
{
  int n;
  bool abort = false;

  while (session->flags & SERVER_SESSION_READY)
  {
    session->request.fields_count = 16;
    n = http_read_request(&session->stream, &session->request.method, &session->request.target, &session->request.body,
                          session->request.fields, &session->request.fields_count);
    if (reactor_unlikely(n == -1))
    {
      server_session_close(session);
      return;
    }
    if (reactor_unlikely(n == 0))
      break;

    session->flags &= ~SERVER_SESSION_READY;
    session->flags |= SERVER_SESSION_PROCESSING;
    session->abort = &abort;
    reactor_call(&session->user, SERVER_REQUEST, (uintptr_t) session);
    if (reactor_unlikely(abort))
      return;
    session->abort = NULL;
    session->flags &= ~SERVER_SESSION_PROCESSING;
  }
  stream_flush(&session->stream);
}

static void server_session_next(reactor_event_t *event)
{
  server_session_t *session = event->state;

  session->next = 0;
  server_session_read(session);
}

inline __attribute__((always_inline, flatten))
static void server_session_stream(reactor_event_t *event)
{
  server_session_t *session = event->state;

  if (reactor_likely(event->type == STREAM_READ))
    server_session_read(session);
  else
    server_session_close(session);
}

static void server_create_session(server_t *server, int fd)
{
  server_session_t *session;

  session = list_push_back(&server->sessions, NULL, sizeof *session);
  session->user = server->user;
  session->flags = SERVER_SESSION_READY;
  stream_construct(&session->stream, server_session_stream, session);
  stream_open(&session->stream, fd, 0);
}

static void server_accept(reactor_event_t *event)
{
  server_t *server = event->state;

  switch (event->type)
  {
  case NETWORK_ACCEPT:
    server_create_session(server, event->data);
    break;
  case NETWORK_ACCEPT_BIND:
      break;
  default:
    server->accept = 0;
    reactor_call(&server->user, SERVER_ERROR, 0);
    break;
  }
}

static void server_timeout(reactor_event_t *event)
{
  (void) event;
  server_date_update();
}

void server_construct(server_t *server, reactor_callback_t *callback, void *state)
{
  *server = (server_t) {.user = reactor_user_define(callback, state)};
  list_construct(&server->sessions);
  timeout_construct(&server->timeout, server_timeout, NULL);
}

void server_destruct(server_t *server)
{
  server_close(server);
  timeout_destruct(&server->timeout);

  while (!list_empty(&server->sessions))
    list_detach(list_front(&server->sessions));
  list_destruct(&server->sessions, NULL);
}

void server_open(server_t *server, const char *host, int port)
{
  server->accept = network_accept(server_accept, server, host, port, NETWORK_REUSEADDR);
  timeout_set(&server->timeout, ((reactor_now() / 1000000000) * 1000000000), 1000000000);
  server_date_update();
}

void server_open_socket(server_t *server, int socket)
{
  server->accept = network_accept_socket(server_accept, server, socket);
  timeout_set(&server->timeout, ((reactor_now() / 1000000000) * 1000000000), 1000000000);
  server_date_update();
}

void server_close(server_t *server)
{
  if (server->accept)
  {
    network_cancel(server->accept);
    server->accept = 0;
  }
  timeout_clear(&server->timeout);
}

void server_disconnect(server_session_t *session)
{
  session->flags |= SERVER_SESSION_READY;
  server_session_close(session);
}

void server_respond(server_session_t *session, string_t status, string_t type, data_t body,
                    http_field_t *fields, size_t fields_count)
{
  session->flags |= SERVER_SESSION_READY;
  if (reactor_likely(stream_is_open(&session->stream)))
  {
    http_write_response(&session->stream, status, data(server_date, 29), type, body, fields, fields_count);
    if (!(session->flags & SERVER_SESSION_PROCESSING))
    {
      stream_flush(&session->stream);
      session->next = reactor_next(server_session_next, session);
    }
  }
  else
  {
    server_session_close(session);
  }
}

void server_ok(server_session_t *session, string_t type, string_t body, http_field_t *fields, size_t fields_count)
{
  server_respond(session, string("200 OK"), type, body, fields, fields_count);
}

void server_plain(server_session_t *session, data_t body, http_field_t *fields, size_t fields_count)
{
  server_respond(session, string("200 OK"), string("text/plain"), body, fields, fields_count);
}
