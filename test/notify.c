#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cmocka.h>
#include <reactor.h>

static void callback_remove(reactor_event_t *event)
{
  notify_t *n = event->state;
  notify_event_t *e = (notify_event_t *) event->data;

  notify_remove_id(n, e->id);
}

static void callback(reactor_event_t *event)
{
  notify_t *n = event->state;

  notify_destruct(n);
  free(n);
}

static void test_notify_basic(__attribute__((unused)) void **arg)
{
  notify_t n;

  /* receive one and remove */
  reactor_construct();
  notify_construct(&n, callback_remove, &n);
  notify_open(&n);
  assert_true(notify_add(&n, "/", IN_ALL_EVENTS) > 0);
  assert_int_equal(closedir(opendir("/")), 0);
  reactor_loop();
  notify_close(&n);
  notify_destruct(&n);
  reactor_destruct();

  /* receive events for file */
  reactor_construct();
  notify_construct(&n, NULL, NULL);
  notify_open(&n);
  assert_true(notify_add(&n, "/bin", IN_ALL_EVENTS) > 0);
  assert_true(notify_add(&n, "/", IN_ALL_EVENTS) > 0);
  assert_int_equal(closedir(opendir("/bin")), 0);
  assert_int_equal(closedir(opendir("/")), 0);
  assert_int_equal(close(open("/bin/sh", O_RDONLY)), 0);
  reactor_loop_once();
  notify_close(&n);
  notify_destruct(&n);
  reactor_destruct();

  /* watch file that does not exist */
  reactor_construct();
  notify_construct(&n, NULL, NULL);
  notify_open(&n);
  assert_true(notify_add(&n, "/invalid/file", IN_ALL_EVENTS) == -1);
  reactor_loop();
  notify_destruct(&n);
  reactor_destruct();
}

static void test_notify_remove(__attribute__((unused)) void **arg)
{
  notify_t n;
  int id;

  /* remove files */
  reactor_construct();
  notify_construct(&n, NULL, NULL);
  notify_open(&n);
  assert_true(notify_add(&n, "/", IN_ALL_EVENTS) > 0);
  assert_true(notify_remove_path(&n, "/"));
  id = notify_add(&n, "/bin", IN_ALL_EVENTS);
  assert_true(notify_remove_id(&n, id));

  /* does not exist */
  assert_false(notify_remove_id(&n, -1));
  assert_false(notify_remove_path(&n, "no"));
  reactor_loop();
  notify_destruct(&n);
  reactor_destruct();

  /* remove and close */
  reactor_construct();
  notify_construct(&n, NULL, NULL);
  notify_open(&n);
  assert_true(notify_add(&n, "/", IN_ALL_EVENTS) > 0);
  assert_true(notify_remove_path(&n, "/"));
  notify_close(&n);
  reactor_loop();
  notify_destruct(&n);
  reactor_destruct();
}

static void test_notify_abort(void **state)
{
  notify_t *n = malloc(sizeof *n);;

  (void) state;
  /* abort while receiving events */
  reactor_construct();
  notify_construct(n, callback, n);
  notify_open(n);
  notify_add(n, "/", IN_ALL_EVENTS);
  notify_add(n, "/", IN_ALL_EVENTS);
  assert_int_equal(closedir(opendir("/")), 0);
  reactor_loop_once();
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_notify_remove),
      cmocka_unit_test(test_notify_basic),
      cmocka_unit_test(test_notify_abort)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
