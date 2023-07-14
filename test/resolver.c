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
  resolver_t r;

  reactor_construct();
  resolver_construct(&r, callback, &state);
  resolver_lookup(&r, 0, 0, 0, 0, "localhost", "http");
  reactor_loop();

  resolver_lookup(&r, 0, 0, 0, 0, "failure", "none");
  reactor_loop();
  resolver_destruct(&r);

  reactor_destruct();
  assert_int_equal(state.calls, 2);
}

static void test_assert(__attribute__((unused)) void **arg)
{
  resolver_t r = {.call = 1};

  expect_assert_failure(resolver_lookup(&r, 0, 0, 0, 0, "localhost", "http"));
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_lookup),
      cmocka_unit_test(test_assert)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
