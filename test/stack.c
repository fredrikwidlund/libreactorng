#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>

#include <value.h>

static void test_stack(__attribute__((unused)) void **arg)
{
  stack s;

  stack_construct(&s);
  assert_true(stack_empty(&s));
  stack_push(&s, NULL);
  assert_false(stack_empty(&s));
  assert_true(stack_peek(&s) == NULL);
  assert_true(stack_pop(&s) == NULL);
  stack_clear(&s, NULL);
  stack_destruct(&s, NULL);
}

int main()
{
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_stack)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
