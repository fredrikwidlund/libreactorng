#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cmocka.h>

#include "reactor.h"

static int server_socket(int *port)
{
  struct sockaddr_in sin = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(0x7f000001)};
  socklen_t len = sizeof sin;
  int s;

  s = socket(AF_INET, SOCK_STREAM, 0);
  assert(s >= 0);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]) {1}, sizeof(int)) == 0);
  assert(bind(s, (struct sockaddr *) &sin, sizeof sin) == 0);
  assert(listen(s, INT_MAX) == 0);
  assert(getsockname(s, (struct sockaddr *) &sin, &len) == 0);
  *port = ntohs(sin.sin_port);
  return s;
}

static int client_socket(int port)
{
  struct sockaddr_in sin = {.sin_family = AF_INET, .sin_addr.s_addr = htonl(0x7f000001), .sin_port = htons(port)};
  socklen_t len = sizeof sin;
  int c;

  c = socket(AF_INET, SOCK_STREAM, 0);
  assert(c >= 0);
  assert(connect(c, (struct sockaddr *) &sin, len) == 0);
  return c;
}

struct state
{
  server_t  server;
  stream_t  stream;
  int       result;
  uint64_t  value;
  size_t    calls;
};

static void server_next1(reactor_event_t *event)
{
  server_session_t *session = (server_session_t *) event->state;

  server_respond(session, string("200 OK"), string("text/plain"), string("ok"), NULL, 0);
}

static void server_next2(reactor_event_t *event)
{
  server_session_t *session = (server_session_t *) event->state;

  server_ok(session, string("text/plain"), string("ok"), NULL, 0);
  server_disconnect(session);
}

static void server_callback(reactor_event_t *event)
{
  struct state *state = event->state;
  server_session_t *session = (server_session_t *) event->data;

  state->calls++;
  assert(event->type == SERVER_REQUEST);
  if (string_equal(session->request.target, string("/abort")))
  {
    server_plain(session, string("no"), NULL, 0);
    server_disconnect(session);
  }
  else if (string_equal(session->request.target, string("/next1")))
    reactor_next(server_next1, session);
  else if (string_equal(session->request.target, string("/next2")))
    reactor_next(server_next2, session);
  else
    server_plain(session, string("ok"), NULL, 0);
}

static void stream_callback(reactor_event_t *event)
{
  struct state *state = event->state;
  data_t data = stream_read(&state->stream);
  int code;

  state->calls++;
  switch (event->type)
  {
  case STREAM_READ:
    assert_true(memchr(data_base(data), '\n', data_size(data)) != NULL);
    assert_true(strncmp(data_base(data), "HTTP/1.1 ", 9) == 0);
    data = data_offset(data, 9);
    code = strtoul(data_base(data), NULL, 0);
    assert_int_equal(code, state->result);
    server_destruct(&state->server);
    stream_close(&state->stream);
    break;
  case STREAM_CLOSE:
    assert_int_equal(state->result, 0);
    server_close(&state->server);
    break;
  }
}

static void run(char *request, int result, int calls)
{
  struct state state = {.result = result};
  int c, s, port;

  reactor_construct();
  server_construct(&state.server, server_callback, &state);

  s = server_socket(&port);
  server_open_socket(&state.server, s);

  stream_construct(&state.stream, stream_callback, &state);
  c = client_socket(port);
  stream_open(&state.stream, c, 0);

  if (request)
  {
    stream_write(&state.stream, data_string(request));
    stream_flush(&state.stream);
  }
  else
  {
    reactor_loop_once();
    shutdown(stream_fd(&state.stream), SHUT_RDWR);
    stream_close(&state.stream);
    reactor_loop_once();
    server_close(&state.server);
  }
  reactor_loop();
  stream_destruct(&state.stream);
  server_destruct(&state.server);
  reactor_destruct();
  close(c);
  close(s);
  assert_int_equal(state.calls, calls);
}

static void test_server(__attribute__((unused)) void **arg)
{
  run("GET / HTTP/1.0\r\n\r\n", 200, 2);
  run("GET / HTTP/1.0\r\n\r\nGET", 200, 2);
  run("GET /abort HTTP/1.0\r\n\r\n", 0, 2);
  run("GET /next1 HTTP/1.0\r\n\r\n", 200, 2);
  run("GET /next2 HTTP/1.0\r\n\r\n", 200, 2);
  run("GET /next1 HTTP/1.0\r\n\r\nGET /next1 HTTP/1.0\r\n\r\n", 200, 3);
  run("i n v a l i d", 0, 1);
  run(NULL, 0, 0);
}

static void test_error(__attribute__((unused)) void **arg)
{
  server_t s1, s2;

  reactor_construct();
  server_construct(&s1, NULL, NULL);
  server_open_socket(&s1, -1);
  server_destruct(&s1);
  reactor_destruct();

  reactor_construct();
  server_construct(&s1, NULL, NULL);
  server_construct(&s2, NULL, NULL);
  server_open(&s1, "127.0.0.1", 12345);
  server_open(&s2, "127.0.0.1", 12345);
  reactor_loop_once();
  server_destruct(&s1);
  server_destruct(&s2);
  reactor_destruct();
}

static void test_basic(__attribute__((unused)) void **arg)
{
  server_t server;

  reactor_construct();
  server_construct(&server, NULL, NULL);
  server_open(&server, "127.0.0.1", 0);
  reactor_loop_once();
  server_destruct(&server);
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_server),
      cmocka_unit_test(test_error),
      cmocka_unit_test(test_basic)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
