#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <cmocka.h>

#include "reactor.h"

static void test_flow(__attribute__((unused)) void **arg)
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

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_flow),
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
