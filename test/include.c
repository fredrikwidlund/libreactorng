#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cmocka.h>

#include <value.h>

static void test_include(__attribute__((unused)) void **arg)
{
  assert_string_equal(VALUE_VERSION, "1.0.0-alpha");
}

int main()
{
  const struct CMUnitTest tests[] = {
      cmocka_unit_test(test_include),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
