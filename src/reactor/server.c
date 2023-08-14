#include <reactor.h>

static void server_session_close(server_session_t *session)
{
  stream_close(&session->stream);
  if (session->next)
  {
    reactor_cancel(session->next, NULL, NULL);
    session->next = 0;
  }
  list_erase(session, NULL);
  // XXX broken, if there is a user holding the session
}

static void server_session_read(server_session_t *session)
{
  int n;
  bool abort = false;

  while (session->flags & SERVER_SESSION_READY)
  {
    n = http_request_read(&session->request, &session->stream);
    if (n <= 0)
    {
      if (n == -1)
      {
        server_session_close(session);
        return;
      }
      break;
    }

    session->flags &= ~SERVER_SESSION_READY;
    session->flags |= SERVER_SESSION_PROCESSING;
    session->abort = &abort;
    reactor_call(&session->server->user, SERVER_REQUEST, (uintptr_t) session);
    if (abort)
      return;
    session->abort = NULL;
    session->flags &= ~SERVER_SESSION_PROCESSING;
  }
  stream_flush(&session->stream);
}

static void server_session_stream(reactor_event_t *event)
{
  server_session_t *session = event->state;

  switch (event->type)
  {
  case STREAM_READ:
    server_session_read(session);
    break;
  default:
    server_session_close(session);
    break;
  }
}

static void server_create_session(server_t *server, int fd)
{
  server_session_t *session;

  session = list_push_back(&server->sessions, NULL, sizeof *session);
  session->server = server;
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

void server_construct(server_t *server, reactor_callback_t *callback, void *state)
{
  *server = (server_t) {.user = reactor_user_define(callback, state)};
  list_construct(&server->sessions);
}

void server_destruct(server_t *server)
{
  server_close(server);

  // XXX tell all sessions to abort
  list_destruct(&server->sessions, NULL);
}

void server_open(server_t *server, const char *host, int port)
{
  server->accept = network_accept(server_accept, server, host, port, NETWORK_REUSEADDR);
}

void server_close(server_t *server)
{
  if (server->accept)
  {
    network_cancel(server->accept);
    server->accept = 0;
  }
}

void server_hello(server_session_t *session)
{
  const data_t data = string("HTTP/1.1 200 OK\r\nServer: Reactor\r\nDate: Sat, 12 Aug 2023 16:00:37 GMT\r\nContent-Type: text/plain\r\nContent-Length: 5\r\n\r\nHello");

  stream_write(&session->stream, data);
  if (!(session->flags & SERVER_SESSION_PROCESSING))
    stream_flush(&session->stream);
  session->flags |= SERVER_SESSION_READY;
}
