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
#include <reactor.h>

struct state
{
  signal_t  signal;
  uint64_t  value;
  size_t    calls;
  bool      close;
};

static void callback(reactor_event_t *event)
{
  struct state *state = event->state;

  state->calls++;
  if (state->close)
    signal_close(&state->signal);
}

static void test_signal(__attribute__((unused)) void **arg)
{
  struct state state = {0};

  reactor_construct();
  signal_construct(&state.signal, callback, &state);
  signal_open(&state.signal);
  signal_send(&state.signal);
  reactor_loop_once();
  assert_int_equal(state.calls, 1);
  signal_close(&state.signal);
  signal_open(&state.signal);
  signal_send_sync(&state.signal);
  state.close = true;
  reactor_loop_once();
  assert_int_equal(state.calls, 2);
  signal_destruct(&state.signal);
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_signal),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
