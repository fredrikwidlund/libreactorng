#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <cmocka.h>

#include "reactor.h"

static void test_start(__attribute__((unused)) void **arg)
{
  flow_t flow;

  reactor_construct();
  flow_construct(&flow, NULL, NULL);
  flow_start(&flow);

  reactor_loop();

  flow_stop(&flow);
  flow_destruct(&flow);
  reactor_destruct();
}

static void test_node(__attribute__((unused)) void **arg)
{
  flow_t flow;

  reactor_construct();
  flow_construct(&flow, NULL, NULL);
  flow_search(&flow, "test/conf");

  /* not found */
  assert_false(flow_node(&flow, "s1", "xxxx", "<main>", value_null()) == 0);

  /* match */
  assert_true(flow_module(&flow, "module1") == 0);
  assert_false(flow_module(&flow, "modulex") == 0);
  flow_module(&flow, "module2");
  assert_true(flow_node(&flow, "module2a", NULL, "<main>", value_null()) == 0);
  assert_false(flow_node(&flow, "module", NULL, "<main>", value_null()) == 0);

  assert_true(flow_node(&flow, "s1", "module1", "<main>", value_null()) == 0);
  assert_true(flow_node(&flow, "s2", "module1", "<main>", value_null()) == 0);
  assert_true(flow_node(&flow, "d1", "module2", "<main>", value_null()) == 0);
  assert_true(flow_node(&flow, "d2", "module2", "<main>", value_null()) == 0);
  flow_connect(&flow, "s1", "s1", value_null());
  flow_connect(&flow, "s1", "d1", value_null());
  flow_connect(&flow, "s2", "d1", value_null());
  flow_connect(&flow, "s1", "d2", value_null());
  flow_connect(&flow, "s2", "d2", value_null());
  flow_connect(&flow, "d1", "d1", value_null());
  assert_false(flow_connect(&flow, "d1", "dx", value_null()) == 0);
  assert_false(flow_connect(&flow, "dx", "d2", value_null()) == 0);
  flow_start(&flow);

  reactor_loop();

  flow_stop(&flow);
  flow_destruct(&flow);
  reactor_destruct();
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_start),
      cmocka_unit_test(test_node)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
