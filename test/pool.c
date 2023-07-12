#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <cmocka.h>

#include "reactor.h"

static void test_pool(__attribute__((unused)) void **arg)
{
  pool_t pool;
  int i, n = 123;
  void *p[n];

  pool_construct(&pool, sizeof (void *));
  assert_int_equal(pool_size(&pool), 0);
  for (i = 0; i < n; i ++)
    p[i] = pool_malloc(&pool);
  assert_int_equal(pool_size(&pool), n);
  for (; i >= 0; i --)
    pool_free(&pool, p[i]);
  pool_destruct(&pool);
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_pool),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
