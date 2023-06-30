#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>

#include <value.h>

static void test_data(__attribute__((unused)) void **arg)
{
  data_t d, d2;

  d = data_null();
  assert_true(data_empty(d));

  d = data_string("test");
  assert_true(strcmp(data_base(d), "test") == 0);
  assert_true(data_equal(d, d));
  assert_true(data_equal(data_offset(d, 2), data_string("st")));
  assert_true(data_equal(data_select(d, 2), data_string("te")));

  assert_false(data_equal(data_string("a"), data_string("b")));
  assert_false(data_equal(data_string("a"), data_string("a1")));

  assert_true(data_equal_case(data_string("aA"), data_string("aA")));
  assert_false(data_equal_case(data_string("a"), data_string("aA")));
  assert_false(data_equal_case(data_string("a"), data_string("b")));

  d2 = data_copy(d);
  assert_true(data_equal(d, d2));
  data_release(d2);
  d2 = data_copyz(d);
  assert_true(data_equal(d, d2));
  data_release(d2);

  d = data_alloc(42);
  d = data_realloc(d, 4711);
  assert_int_equal(data_size(d), 4711);
  data_release(d);
}

int main()
{
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_data)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
