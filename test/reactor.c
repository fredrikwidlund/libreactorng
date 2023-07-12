#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cmocka.h>
#include <linux/version.h>

#include <reactor.h>

struct state
{
  bool     ignore_value;
  uint64_t value;
  size_t   calls;
};

static void callback(reactor_event_t *event)
{
  struct state *state = event->state;

  if (!state->ignore_value)
    assert_int_equal((int) event->data, state->value);
  state->calls++;
}

static void test_construct(__attribute__((unused)) void **arg)
{
  reactor_construct();
  reactor_construct();
}

static void test_destruct(__attribute__((unused)) void **arg)
{
  reactor_destruct();
  reactor_destruct();
}

static void test_now(__attribute__((unused)) void **arg)
{
  assert_true(reactor_now() == reactor_now());
}

static void test_loop(__attribute__((unused)) void **arg)
{
  reactor_loop();
  reactor_loop_once();
}

static void test_async(__attribute__((unused)) void **arg)
{
  struct state state = {0};

  reactor_async(callback, &state);
  reactor_loop();
  assert_int_equal(state.calls, 2);
}

static void test_next(__attribute__((unused)) void **arg)
{
  reactor_next(NULL, NULL);
  reactor_loop();
}

static void test_cancel(__attribute__((unused)) void **arg)
{
  /* cancel nop call */
  reactor_cancel(reactor_nop(NULL, NULL), NULL, NULL);
  /* cancel local next call */
  reactor_cancel(reactor_next(NULL, NULL), NULL, NULL);
  reactor_loop();
}

static void test_nop(__attribute__((unused)) void **arg)
{
  reactor_nop(NULL, NULL);
  reactor_loop();
}

static void test_readv_writev(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 4};
  char buffer[4] = "test";
  struct iovec iov = {.iov_base = buffer, .iov_len = sizeof buffer};
  int fd[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
  reactor_writev(callback, &state, fd[1], &iov, 1, 0);
  reactor_loop();

  buffer[0] = 0;

  reactor_readv(callback, &state, fd[0], &iov, 1, 0);
  reactor_loop();
  close(fd[0]);
  close(fd[1]);

  assert_int_equal(state.calls, 2);
  assert_string_equal(buffer, "test");
}

static void test_fsync(__attribute__((unused)) void **arg)
{
  reactor_fsync(NULL, NULL, 0);
  reactor_loop();
}

static void test_poll(__attribute__((unused)) void **arg)
{
  struct state state = {.ignore_value = true};
  int fd[2];
  reactor_id_t id;

  /* remove */
  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
  id = reactor_poll_add(callback, &state, fd[0], POLLOUT);
  reactor_poll_remove(callback, &state, id);
  reactor_loop();
  assert_int_equal(state.calls, 2);

  /* add multi poll */
  reactor_write(NULL, NULL, fd[1], "", 1, 0);
  id = reactor_poll_add_multi(callback, &state, fd[0], POLLIN);
  reactor_loop_once();
  assert_int_equal(state.calls, 3);

  /* receive multiple events */
  reactor_write(NULL, NULL, fd[1], "", 1, 0);
  reactor_loop_once();
  assert_int_equal(state.calls, 4);

  /* remove multi */
  reactor_poll_remove(callback, &state, id);
  reactor_loop();
  assert_int_equal(state.calls, 6);

  /* update events */
  id = reactor_poll_add_multi(callback, &state, fd[0], POLLIN);
  reactor_loop_once();
  reactor_poll_update(NULL, NULL, id, POLLOUT);
  state.ignore_value = false;
  state.value = 4;
  reactor_loop_once();

  /* remove multi */
  reactor_poll_remove(NULL, NULL, id);
  state.ignore_value = true;
  state.value = 0;
  reactor_loop();

  close(fd[0]);
  close(fd[1]);
}

static void test_epoll_ctl(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 0};
  int epoll_fd, fd[2];
  struct epoll_event event = {.events = EPOLLOUT | EPOLLET};

  epoll_fd = epoll_create(64);
  assert_true(epoll_fd >= 0);
  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
  reactor_epoll_ctl(callback, &state, epoll_fd, EPOLL_CTL_ADD, fd[0], &event);
  reactor_loop();
  assert_true(epoll_wait(epoll_fd, &event, 1, 0) == 1);
  assert_int_equal(event.events, EPOLLOUT);
  close(fd[0]);
  close(fd[1]);
  close(epoll_fd);
  assert_int_equal(state.calls, 1);
}

static void test_sync_file_range(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 0};
  FILE *f = tmpfile();

  assert_true(f);
  reactor_sync_file_range(callback, &state, fileno(f), 0, 0, 0);
  reactor_loop();
  assert_int_equal(state.calls, 1);
  assert_true(fclose(f) == 0);
}

static void test_recvmsg_sendmsg(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 4};
  char buffer[4] = "test";
  struct iovec iov = {.iov_base = buffer, .iov_len = sizeof buffer};
  struct msghdr message = {.msg_iov = &iov, .msg_iovlen = 1};
  int fd[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
  reactor_sendmsg(callback, &state, fd[1], &message, 0);
  reactor_loop();

  buffer[0] = 0;

  reactor_recvmsg(callback, &state, fd[0], &message, 0);
  reactor_loop();
  close(fd[0]);
  close(fd[1]);

  assert_int_equal(state.calls, 2);
  assert_string_equal(buffer, "test");
}

static void test_send(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 4};
  int fd[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
  reactor_send(callback, &state, fd[1], "test", 4, 0);
  reactor_loop();
  assert_int_equal(state.calls, 1);
  close(fd[0]);
  close(fd[1]);
}

static void test_recv(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 6};
  static char buffer[1024] = {0};
  int fd[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
  assert(write(fd[1], "test2", 6) == 6);
  reactor_recv(callback, &state, fd[0], buffer, sizeof buffer, 0);
  reactor_loop();
  assert_int_equal(state.calls, 1);
  close(fd[0]);
  close(fd[1]);
}

static void test_timeout(__attribute__((unused)) void **arg)
{
  struct state state = {.value = -ETIME};
  static struct timespec tv = {0};
  int i;

  reactor_timeout(NULL, NULL, &tv, 0, 0);
  reactor_loop();
  assert_int_equal(state.calls, 0);

  reactor_timeout(callback, &state, &tv, 0, 0);
  reactor_loop();
  assert_int_equal(state.calls, 1);

  for (i = 0; i < REACTOR_RING_SIZE + 1; i++)
    reactor_timeout(callback, &state, &tv, 0, 0);
  reactor_loop();
  assert_int_equal(state.calls, REACTOR_RING_SIZE + 1 + 1);
}

static void test_accept_callback(reactor_event_t *event)
{
  int *calls = event->state;
  int fd = event->data;

  assert_true(fd >= 0);
  (*calls)++;
}

static void test_accept(__attribute__((unused)) void **arg)
{
  int s, c, calls = 0;
  struct sockaddr_in sin;
  socklen_t len = sizeof sin;

  s = socket(AF_INET, SOCK_STREAM, 0);
  assert(s >= 0);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]) {1}, sizeof(int)) == 0);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]) {1}, sizeof(int)) == 0);
  assert(bind(s, (struct sockaddr *) (struct sockaddr_in[]) {{.sin_family = AF_INET}}, sizeof(struct sockaddr_in)) == 0);
  assert(listen(s, INT_MAX) == 0);
  assert(getsockname(s, (struct sockaddr *) &sin, &len) == 0);

  c = socket(AF_INET, SOCK_STREAM, 0);
  assert(c >= 0);
  assert(connect(c, (struct sockaddr *) &sin, len) == 0);

  reactor_accept(test_accept_callback, &calls, s, NULL, NULL, 0);
  reactor_loop();
  assert_int_equal(calls, 1);
  close(s);
  close(c);
}

static void test_read(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 6};
  static char buffer[1024] = {0};
  int fd[2];

  assert(pipe(fd) == 0);
  assert(write(fd[1], "test2", 6) == 6);
  reactor_read(callback, &state, fd[0], buffer, sizeof buffer, 0);
  reactor_loop();
  assert_int_equal(state.calls, 1);
  close(fd[0]);
  close(fd[1]);
}

static void test_write(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 4};
  int fd[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
  reactor_write(callback, &state, fd[1], "test", 4, 0);
  reactor_loop();
  assert_int_equal(state.calls, 1);
  close(fd[0]);
  close(fd[1]);
}

static void test_connect(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 0};
  struct sockaddr_in sin;
  socklen_t len = sizeof sin;
  int s, c;

  s = socket(AF_INET, SOCK_STREAM, 0);
  assert(s >= 0);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]) {1}, sizeof(int)) == 0);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]) {1}, sizeof(int)) == 0);
  assert(bind(s, (struct sockaddr *) (struct sockaddr_in[]) {{.sin_family = AF_INET}}, sizeof(struct sockaddr_in)) == 0);
  assert(listen(s, INT_MAX) == 0);
  assert(getsockname(s, (struct sockaddr *) &sin, &len) == 0);

  c = socket(AF_INET, SOCK_STREAM, 0);
  assert(c >= 0);

  reactor_connect(callback, &state, c, (struct sockaddr *) &sin, len);
  reactor_loop();
  assert_int_equal(state.calls, 1);
  close(s);
  close(c);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
static void test_send_zerocopy(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 0};
  struct sockaddr_in sin;
  socklen_t len = sizeof sin;
  int s, c;

  s = socket(AF_INET, SOCK_STREAM, 0);
  assert(s >= 0);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEPORT, (int[]) {1}, sizeof(int)) == 0);
  assert(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (int[]) {1}, sizeof(int)) == 0);
  assert(bind(s, (struct sockaddr *) (struct sockaddr_in[]) {{.sin_family = AF_INET}}, sizeof(struct sockaddr_in)) == 0);
  assert(listen(s, INT_MAX) == 0);
  assert(getsockname(s, (struct sockaddr *) &sin, &len) == 0);

  c = socket(AF_INET, SOCK_STREAM, 0);
  assert(c >= 0);

  reactor_connect(callback, &state, c, (struct sockaddr *) &sin, len);
  reactor_loop_once();
  assert_int_equal(state.calls, 1);
  state.value = 4;
  reactor_send_zerocopy(callback, &state, c, "test", 4, 0);
  reactor_loop();
  assert_int_equal(state.calls, 2);
  close(s);
  close(c);
}
#endif

static void test_fallocate(__attribute__((unused)) void **arg)
{
  struct state state = {.value = 0};
  FILE *f = tmpfile();

  assert_true(f);
  reactor_fallocate(callback, &state, fileno(f), 0, 0, 4096);
  reactor_loop();
  assert_int_equal(state.calls, 1);
  assert_true(fclose(f) == 0);
}

static void test_close(__attribute__((unused)) void **arg)
{
  int fd[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
  reactor_close(NULL, NULL, fd[0]);
  reactor_close(NULL, NULL, fd[1]);
  reactor_loop();
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_construct),
      cmocka_unit_test(test_now),
      cmocka_unit_test(test_loop),
      cmocka_unit_test(test_async),
      cmocka_unit_test(test_next),
      cmocka_unit_test(test_cancel),
      cmocka_unit_test(test_nop),
      cmocka_unit_test(test_readv_writev),
      cmocka_unit_test(test_fsync),
      cmocka_unit_test(test_poll),
      cmocka_unit_test(test_epoll_ctl),
      cmocka_unit_test(test_sync_file_range),
      cmocka_unit_test(test_recvmsg_sendmsg),
      cmocka_unit_test(test_send),
      cmocka_unit_test(test_recv),
      cmocka_unit_test(test_timeout),
      cmocka_unit_test(test_accept),
      cmocka_unit_test(test_read),
      cmocka_unit_test(test_write),
      cmocka_unit_test(test_connect),
      cmocka_unit_test(test_fallocate),
      cmocka_unit_test(test_close),
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
      cmocka_unit_test(test_send_zerocopy),
#endif
      cmocka_unit_test(test_destruct)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
