#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <cmocka.h>

#include <value.h>

void test_buffer(__attribute__((unused)) void **state)
{
  buffer_t b;
  char *p;
  FILE *f;

  buffer_construct(&b);
  assert_int_equal(buffer_size(&b), 0);
  assert_int_equal(buffer_capacity(&b), 0);

  buffer_resize(&b, 100);
  assert_int_equal(buffer_size(&b), 100);
  buffer_resize(&b, 0);
  assert_int_equal(buffer_size(&b), 0);

  buffer_reserve(&b, 0);
  buffer_reserve(&b, 1024);
  assert_int_equal(buffer_capacity(&b), 1024);

  buffer_compact(&b);
  buffer_compact(&b);
  assert_int_equal(buffer_capacity(&b), 0);

  buffer_prepend(&b, data_string("first"));
  buffer_append(&b, data_string("last"));
  buffer_insert(&b, buffer_size(&b), data_define("", 1));
  assert_string_equal(buffer_base(&b), "firstlast");

  buffer_erase(&b, 5, 4);
  assert_string_equal(buffer_base(&b), "first");

  buffer_insert_fill(&b, 0, 5, data_string("x"));
  assert_string_equal(buffer_base(&b), "xxxxxfirst");
  buffer_insert_fill(&b, buffer_size(&b), 5, data_string("x"));
  assert_string_equal(buffer_base(&b), "xxxxxfirst");

  p = data_base(buffer_data(&b));
  assert_string_equal(p, "xxxxxfirst");
  buffer_destruct(&b);

  buffer_construct(&b);
  p = buffer_deconstruct(&b);
  free(p);

  buffer_construct(&b);
  assert_true(buffer_empty(&b));
  assert_true(data_empty(buffer_data(&b)));
  assert_true(buffer_end(&b) == buffer_base(&b));
  assert_true(data_base(buffer_allocate(&b, 256)) != NULL);
  buffer_clear(&b);
  buffer_destruct(&b);

  buffer_construct(&b);
  assert_false(buffer_load(&b, "/does/not/exist"));
  assert_true(buffer_load(&b, "/dev/null"));
  assert_true(data_equal(buffer_data(&b), data_null()));
  assert_false(buffer_loadz(&b, "/does/not/exist"));
  buffer_loadz(&b, "/dev/null");
  assert_true(data_equal(buffer_data(&b), data_null()));
  assert_true(buffer_save(&b, "/dev/null"));
  assert_false(buffer_save(&b, "/does/not/exist"));
  buffer_destruct(&b);

  f = tmpfile();
  buffer_construct(&b);
  buffer_insert_fill(&b, 0, 65536, data_define("", 1));
  buffer_write(&b, f);
  buffer_clear(&b);
  fseek(f, 0, SEEK_SET);
  buffer_read(&b, f);
  assert_int_equal(buffer_size(&b), 65536);
  buffer_destruct(&b);
  fclose(f);
}

int main()
{
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_buffer)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
