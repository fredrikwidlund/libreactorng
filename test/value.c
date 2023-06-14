#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>
#include <value.h>

static void counter_destroy(void *object)
{
  int **counter = object;

  (**counter)++;
}

static void test_value(__attribute__((unused)) void **arg)
{
  static const value_table_t counter_table = {.destroy = counter_destroy};
  static const value_table_t user_table = {.user = 4711};
  ;
  value_t *v;
  int counter = 0, *p;

  v = NULL;
  assert_int_equal(value_ref(v), 0);
  assert_int_equal(value_user(v), 0);
  value_hold(v);
  assert_int_equal(value_ref(v), 0);
  value_release(v);
  assert_int_equal(value_ref(v), 0);

  v = value_create(NULL, 0, NULL);
  assert_int_equal(value_user(v), 0);
  assert_int_equal(value_ref(v), 1);
  value_release(v);

  v = value_create((int[]) {42}, sizeof(int), NULL);
  assert_int_equal(*(int *) v, 42);
  value_hold(v);
  assert_int_equal(value_ref(v), 2);
  value_release(v);
  value_release(v);

  p = &counter;
  v = value_create(&p, sizeof p, &counter_table);
  assert_int_equal(value_user(v), 0);
  value_release(v);
  assert_int_equal(counter, 1);

  v = value_create(NULL, 0, &user_table);
  assert_int_equal(value_user(v), 4711);
  value_release(v);
}

static void test_undefined(__attribute__((unused)) void **arg)
{
  value_t *v;

  v = value_undefined();
  value_hold(v);
  value_release(v);
  assert_int_equal(value_ref(v), 0);
  assert_int_equal(value_user(v), 0);
  assert_int_equal(value_type(v), VALUE_UNDEFINED);
  assert_true(value_undefinedp(v));
  assert_false(value_undefinedp(value_null()));
  value_release(v);
}

static void test_null(__attribute__((unused)) void **arg)
{
  value_t *v;

  v = value_null();
  value_hold(v);
  value_release(v);
  assert_int_equal(value_ref(v), 0);
  assert_int_equal(value_type(v), VALUE_NULL);
  assert_true(value_nullp(v));
  assert_false(value_nullp(value_undefined()));
  value_release(v);
}

static void test_bool(__attribute__((unused)) void **arg)
{
  value_t *v;

  v = value_bool(true);
  value_hold(v);
  value_release(v);
  assert_int_equal(value_ref(v), 1);
  assert_int_equal(value_type(v), VALUE_BOOL);
  assert_true(value_boolp(v));
  assert_false(value_boolp(value_undefined()));
  assert_true(value_bool_get(v));
  assert_false(value_bool_get(value_null()));
  value_release(v);

  v = value_bool(false);
  assert_false(value_bool_get(v));
  value_release(v);
}

static void test_number(__attribute__((unused)) void **arg)
{
  value_t *v;

  v = value_number(3.14159);
  value_hold(v);
  value_release(v);
  assert_int_equal(value_ref(v), 1);
  assert_int_equal(value_type(v), VALUE_NUMBER);
  assert_true(value_numberp(v));
  assert_false(value_numberp(value_undefined()));
  assert_true(value_number_get(v) == 3.14159);
  assert_true(value_number_get(value_undefined()) == 0);
  value_release(v);
}

static void test_string(__attribute__((unused)) void **arg)
{
  value_t *v;

  v = value_string(string("test"));
  value_hold(v);
  value_release(v);
  assert_int_equal(value_ref(v), 1);
  assert_int_equal(value_type(v), VALUE_STRING);
  assert_true(value_stringp(v));
  assert_false(value_stringp(value_undefined()));
  assert_string_equal(string_base(value_string_get(v)), "test");
  assert_true(string_empty(value_string_get(value_undefined())));
  value_release(v);

  v = value_string_constant(string("test"));
  assert_string_equal(value_string_base(v), "test");
  assert_int_equal(value_string_size(v), strlen("test"));
  value_release(v);

  v = value_string_release(string_copy(string("test")));
  assert_string_equal(value_string_base(v), "test");
  assert_int_equal(value_string_size(v), strlen("test"));
  value_release(v);
}

static void test_array(__attribute__((unused)) void **arg)
{
  value_t *v, *e;

  v = value_undefined();
  assert_false(value_arrayp(v));
  assert_int_equal(value_array_length(v), 0);
  e = value_number(42);
  value_array_append(v, e);
  value_release(e);
  value_array_append_release(v, value_number(42));
  assert_true(value_undefinedp(value_array_get(v, 0)));
  value_array_remove(v, 0);
  value_release(v);

  v = value_array();
  assert_true(value_arrayp(v));
  value_hold(v);
  value_release(v);
  e = value_number(42);
  value_array_append(v, e);
  value_release(e);
  value_array_append_release(v, value_number(43));
  assert_int_equal(value_number_get(value_array_get(v, 0)), 42);
  assert_int_equal(value_number_get(value_array_get(v, 1)), 43);
  value_array_remove(v, 0);
  assert_int_equal(value_number_get(value_array_get(v, 0)), 43);
  value_release(v);
}

static void test_object(__attribute__((unused)) void **arg)
{
  value_t *v, *array, *e, *keys;

  v = value_undefined();
  value_object_set_release(v, string("test"), value_number(0));
  value_object_delete(v, string("test"));
  assert_true(value_undefinedp(value_object_get(v, string("test"))));
  assert_int_equal(value_object_size(v), 0);
  assert_false(value_objectp(v));

  array = value_object_keys(v);
  assert_int_equal(value_array_length(array), 0);
  value_release(array);

  v = value_object();
  value_hold(v);
  value_release(v);
  assert_int_equal(value_ref(v), 1);
  assert_int_equal(value_type(v), VALUE_OBJECT);
  assert_true(value_objectp(v));

  value_object_set_release(v, string("k1"), value_number(42));
  value_object_set_release(v, string("k2"), value_bool(false));
  value_object_delete(v, string("k2"));
  value_object_set_release(v, string("k3"), value_string(string("text")));
  value_object_set_release(v, string("k3"), value_string(string("text2")));
  value_object_set_release(v, string("k4a"), value_string(string("text3a")));
  value_object_set_release(v, string("k4"), value_string(string("text3")));
  value_object_set_release(v, string("k5"), value_string(string("text4a")));
  value_object_set_release(v, string("k5a"), value_string(string("text4")));
  assert_int_equal(value_object_size(v), 6);

  e = value_object_get(v, string("k3"));
  assert_true(value_stringp(e));
  assert_string_equal(value_string_base(e), "text2");
  assert_true(value_undefinedp(value_object_get(v, string("k6"))));

  keys = value_object_keys(v);
  assert_int_equal(value_array_length(keys), 6);
  assert_string_equal(value_string_base(value_array_get(keys, 0)), "k1");
  assert_string_equal(value_string_base(value_array_get(keys, 1)), "k3");
  assert_string_equal(value_string_base(value_array_get(keys, 2)), "k4");
  assert_string_equal(value_string_base(value_array_get(keys, 3)), "k4a");
  assert_string_equal(value_string_base(value_array_get(keys, 4)), "k5");
  value_release(keys);
  value_release(v);

  e = value_bool(true);
  v = value_object();
  value_object_set_release(v, string("key"), e);
  e = v;
  v = value_object();
  value_object_set_release(v, string("key"), e);
  e = v;
  v = value_object();
  value_object_set_release(v, string("key"), e);
  value_release(v);
}

int main()
{
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_value),  cmocka_unit_test(test_undefined),
                                     cmocka_unit_test(test_null),   cmocka_unit_test(test_bool),
                                     cmocka_unit_test(test_number), cmocka_unit_test(test_string),
                                     cmocka_unit_test(test_array),  cmocka_unit_test(test_object)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
