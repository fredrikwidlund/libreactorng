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

static void test_resolve(__attribute__((unused)) void **arg)
{
  struct state state = {0};
  network_t id;

  reactor_construct();
  network_expire(1000000ULL);

  id = network_resolve(callback, &state, "127.0.0.1");
  network_cancel(id);
  id = network_resolve(callback, &state, "fail.localdomain");
  network_cancel(id);
  reactor_loop();

  network_resolve(callback, &state, "127.0.0.1");
  network_resolve(callback, &state, "fail.localdomain");
  network_resolve(callback, &state, "fail.localdomain");
  reactor_loop();

  network_resolve(callback, &state, NULL);
  reactor_loop();

  /* reuse cached */
  network_resolve(callback, &state, NULL);
  reactor_loop();

  /* expired */
  usleep(10000);
  network_resolve(callback, &state, NULL);
  reactor_loop();

  reactor_destruct();

  assert_int_equal(state.calls, 6);
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_resolve)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
