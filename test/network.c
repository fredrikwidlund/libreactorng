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
  uint64_t value;
  size_t   calls;
};

static void callback(reactor_event_t *event)
{
  struct state *state = event->state;

  state->calls++;
}

static void test_lookup(__attribute__((unused)) void **arg)
{
  struct state state = {0};
  network_t id;

  reactor_construct();
  network_expire(1000000ULL);

  id = network_resolve(callback, &state, "127.0.0.1");
  network_cancel(id);
  id = network_resolve(callback, &state, "does.not.exist");
  network_cancel(id);
  reactor_loop();

  network_resolve(callback, &state, "127.0.0.1");
  network_resolve(callback, &state, "does.not.exist");
  network_resolve(callback, &state, "does.not.exist");
  reactor_loop();

  network_resolve(callback, &state, "www.google.com");
  reactor_loop();

  /* reuse cached */
  network_resolve(callback, &state, "www.google.com");
  reactor_loop();

  /* expired */
  usleep(10000);
  network_resolve(callback, &state, "www.google.com");
  reactor_loop();

  /* expire */
  reactor_destruct();

  assert_int_equal(state.calls, 6);
}

/*
static void test_assert(__attribute__((unused)) void **arg)
{
  resolver_t r = {.call = 1};

  expect_assert_failure(network_resolve(&r, 0, 0, 0, 0, "localhost", "http"));
}
*/

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_lookup)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
