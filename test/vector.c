#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>
#include <reactor.h>

static void release(void *object)
{
  free(*(char **) object);
}

static int compare(const void *p1, const void *p2)
{
  const char *s1 = *(char **) p1, *s2 = *(char **) p2;

  return strcmp(s1, s2);
}

void test_vector(__attribute__((unused)) void **state)
{
  vector_t v;
  char *a[] = {"a", "list", "of", "string", "pointers"}, *s;
  size_t a_len, i;

  vector_construct(&v, sizeof(char *));
  assert_true(vector_empty(&v));
  assert_int_equal(vector_size(&v), 0);
  assert_int_equal(vector_capacity(&v), 0);

  vector_reserve(&v, 1024);
  assert_int_equal(vector_capacity(&v), 1024);
  vector_shrink_to_fit(&v);
  assert_int_equal(vector_capacity(&v), 0);

  vector_insert_fill(&v, 0, 5, (char *[]) {"foo"});
  for (i = 0; i < 5; i++)
    assert_string_equal("foo", *(char **) vector_at(&v, i));
  vector_erase_range(&v, 0, 5, NULL);

  a_len = sizeof a / sizeof a[0];
  vector_insert_range(&v, 0, &a[0], &a[a_len]);
  for (i = 0; i < a_len; i++)
    assert_string_equal(a[i], *(char **) vector_at(&v, i));

  assert_string_equal(a[0], *(char **) vector_front(&v));
  assert_string_equal(a[a_len - 1], *(char **) vector_back(&v));
  vector_erase(&v, 0, NULL);
  assert_string_equal(a[1], *(char **) vector_front(&v));

  vector_push_back(&v, (char *[]) {"pushed"});
  assert_string_equal("pushed", *(char **) vector_back(&v));
  vector_pop_back(&v, NULL);
  assert_string_equal(a[a_len - 1], *(char **) vector_back(&v));
  vector_clear(&v, NULL);

  for (i = 0; i < a_len; i++)
  {
    s = malloc(strlen(a[i]) + 1);
    strcpy(s, a[i]);
    vector_insert(&v, i, &s);
  }
  for (i = 0; i < a_len; i++)
    assert_string_equal(a[i], *(char **) vector_at(&v, i));

  vector_sort(&v, compare);
  assert_string_equal(*(char **) vector_at(&v, 3), "pointers");
  assert_string_equal(*(char **) vector_at(&v, 4), "string");

  vector_erase(&v, 0, release);
  assert_string_equal(a[1], *(char **) vector_front(&v));

  vector_destruct(&v, release);
}

int main()
{
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_vector)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
