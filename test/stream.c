#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <sys/socket.h>
#include <cmocka.h>

#include "reactor.h"

struct state
{
  uint64_t value;
  size_t   calls;
  size_t   read;
  stream_t in;
  stream_t out;
  size_t   errors;
};

static void callback(reactor_event_t *event)
{
  struct state *state = event->state;
  data_t data;

  state->calls++;

  switch (event->type)
  {
  case STREAM_READ:
    data = stream_read(&state->in);
    state->read += data_size(data);
    if (state->read >= 65536)
      stream_consume(&state->in, data_size(data));
    if (state->read >= 65536 * 2)
      stream_close(&state->in);
    break;
  case STREAM_ERROR:
    state->errors++;
    break;
  default:
    break;
  }
}

static void test_domain_socket(__attribute__((unused)) void **arg)
{
  struct state state = {0};
  int fd[2];

  assert_true(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);

  reactor_construct();
  stream_construct(&state.in, callback, &state);
  stream_open(&state.in, fd[0], 0);
  stream_construct(&state.out, callback, &state);
  stream_open(&state.out, fd[1], 0);
  stream_write(&state.out, data_string("test"));
  stream_flush(&state.out);
  stream_flush(&state.out);
  stream_write(&state.out, data_string("test"));
  stream_flush(&state.out);
  reactor_loop_once();
  stream_close(&state.in);
  reactor_loop();
  stream_destruct(&state.in);
  stream_destruct(&state.out);
  reactor_destruct();
  assert_int_equal(state.calls, 2);
  assert_int_equal(state.errors, 0);
  close(fd[0]);
  close(fd[1]);
}

static void test_pipe(__attribute__((unused)) void **arg)
{
  struct state state = {0};
  char chunk[1024 * 1024] = {0};
  int fd[2];

  assert_true(pipe(fd) == 0);

  reactor_construct();
  stream_construct(&state.in, callback, &state);
  stream_open(&state.in, fd[0], 0);
  stream_construct(&state.out, callback, &state);
  stream_open(&state.out, fd[1], STREAM_WRITE_ONLY);
  stream_write(&state.out, data(chunk, sizeof chunk));
  stream_flush(&state.out);
  stream_flush(&state.out);
  reactor_loop_once();
  stream_close(&state.out);
  reactor_loop();
  stream_destruct(&state.in);
  stream_destruct(&state.out);
  reactor_destruct();
  assert_true(state.calls >= 2);
  assert_int_equal(state.errors, 0);
  close(fd[0]);
  close(fd[1]);
}

static void test_error(__attribute__((unused)) void **arg)
{
  struct state state = {0};
  int fd[2];

  assert_true(pipe(fd) == 0);

  /* read from write only fd, and write to read only fd */
  reactor_construct();
  stream_construct(&state.in, callback, &state);
  stream_construct(&state.out, callback, &state);
  stream_open(&state.in, fd[0], 0);
  stream_write(&state.in, data("test", 4));
  stream_flush(&state.in);
  reactor_loop_once();
  stream_close(&state.in);
  stream_open(&state.out, fd[1], 0);
  reactor_loop();
  stream_destruct(&state.in);
  stream_destruct(&state.out);
  reactor_destruct();
  assert_int_equal(state.errors, 2);
  close(fd[0]);
  close(fd[1]);
}


int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_domain_socket),
      cmocka_unit_test(test_pipe),
      cmocka_unit_test(test_error)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
