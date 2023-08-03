#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cmocka.h>

#include "reactor.h"

struct state
{
  network_t id;
  bool cancel;
  int port;
};

static void test_accept_callback(reactor_event_t *event)
{
  struct state *state = event->state;
  int e, fd = event->data;
  struct sockaddr_in sin;
  socklen_t len = sizeof sin;

  switch (event->type)
  {
  case NETWORK_ACCEPT_BIND:
    assert_true(fd >= 0);
    if (state->cancel)
    {
      network_cancel(state->id);
      break;
    }
    e = getsockname(fd, (struct sockaddr *) &sin, &len);
    assert_int_equal(e, 0);
    state->port = ntohs(sin.sin_port);
    network_connect(test_accept_callback, NULL, "127.0.0.1", state->port);
    break;
  case NETWORK_ACCEPT:
    assert_true(fd >= 0);
    (void) close(fd);
    network_cancel(state->id);
    break;
  case NETWORK_CONNECT:
    assert_true(fd >= 0);
    (void) close(fd);
    break;
  default:
    assert_false(true);
  }
}

static void test_accept(__attribute__((unused)) void **arg)
{
  struct state state = {0};

  /* specify no port and connect */
  reactor_construct();
  state.id = network_accept(test_accept_callback, &state, "127.0.0.1", 0, NETWORK_REUSEADDR | NETWORK_REUSEPORT);
  reactor_loop();
  reactor_destruct();

  /* specify ipv4 port */
  reactor_construct();
  state.cancel = true;
  state.id = network_accept(test_accept_callback, &state, "127.0.0.1", state.port, NETWORK_REUSEADDR);
  reactor_loop();
  reactor_destruct();

  /* specify ipv6 port */
  reactor_construct();
  state.id = network_accept(test_accept_callback, &state, "::1", state.port, NETWORK_REUSEADDR);
  reactor_loop();
  reactor_destruct();
}

static void test_connect_next(reactor_event_t *event)
{
  network_t *id = event->state;

  network_cancel(*id);
}

static void test_connect(__attribute__((unused)) void **arg)
{
  network_t id;

  /* connect and cancel while connecting */
  reactor_construct();
  id = network_connect(NULL, NULL, "127.0.0.1", 0);
  reactor_next(test_connect_next, &id);
  reactor_loop();
  reactor_destruct();
}

static void test_cancel(__attribute__((unused)) void **arg)
{
  network_t id;

  reactor_construct();
  id = network_accept(NULL, NULL, "1.2.3.4", 80, 0);
  network_cancel(id);
  id = network_connect(NULL, NULL, "1.2.3.4", 80);
  network_cancel(id);
  reactor_loop();
  reactor_destruct();
}

static void test_error_callback(reactor_event_t *event)
{
  int fd = event->data, *errors = event->state;

  switch (event->type)
  {
  case NETWORK_ACCEPT_BIND:
    close(fd);
    break;
  case NETWORK_ERROR:
    (*errors)++;
    break;
  }
}

static void test_error(__attribute__((unused)) void **arg)
{
  struct sockaddr_in sin = {.sin_family = 99};
  int errors = 0;

  // error on socket()
  reactor_construct();
  network_connect_address(test_error_callback, &errors, (struct sockaddr *) &sin, sizeof sin);
  network_accept_address(test_error_callback, &errors, (struct sockaddr *) &sin, sizeof sin, 0);
  reactor_loop();
  assert_int_equal(errors, 2);

  // error on bind();
  errors = 0;
  sin.sin_addr.s_addr = ntohl(0x12345678);
  sin.sin_family = AF_INET;
  network_accept_address(test_error_callback, &errors, (struct sockaddr *) &sin, sizeof sin, 0);
  reactor_loop();
  assert_int_equal(errors, 1);

  // error on accept since socket is closed;
  errors = 0;
  network_accept(test_error_callback, &errors, "127.0.0.1", 0, NETWORK_REUSEADDR);
  reactor_loop();
  assert_int_equal(errors, 1);

  // error on accept resolve;
  errors = 0;
  network_accept(test_error_callback, &errors, "999.999", 0, 0);
  reactor_loop();
  assert_int_equal(errors, 1);

  // error on accept bind;
  errors = 0;
  network_accept(test_error_callback, &errors, "12.34.56.78", 0, 0);
  reactor_loop();
  assert_int_equal(errors, 1);

  /* specify socket */
  errors = 0;
  network_accept_socket(test_error_callback, &errors, -1);
  reactor_loop();
  assert_int_equal(errors, 1);

  /* connect error */
  errors = 0;
  network_connect(test_error_callback, &errors, "127.0.0.123", 4567);
  reactor_loop();
  assert_int_equal(errors, 1);

  /* connect error */
  errors = 0;
  network_connect(test_error_callback, &errors, "::2", 4567);
  reactor_loop();
  assert_int_equal(errors, 1);

  // error on connect resolve;
  errors = 0;
  network_connect(test_error_callback, &errors, "999.999", 0);
  reactor_loop();
  assert_int_equal(errors, 1);

  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_accept),
      cmocka_unit_test(test_connect),
      cmocka_unit_test(test_cancel),
      cmocka_unit_test(test_error)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
