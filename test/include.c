#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cmocka.h>
#include <reactor.h>

static void test_include(__attribute__((unused)) void **arg)
{
  assert_string_equal(REACTOR_VERSION, "0.9.2");
}

int main()
{
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_include),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
