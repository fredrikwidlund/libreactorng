#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <setjmp.h>
#include <cmocka.h>
#include <reactor.h>

static void release(mapd_entry_t *e)
{
  free(data_base(e->key));
}

void test_mapd(__attribute__((unused)) void **state)
{
  mapd_t mapd;
  int i;
  void *p;
  char key[16];

  mapd_construct(&mapd);

  for (i = 0; i < 2; i++)
  {
    p = malloc(5);
    printf("-> %p\n", p);
    strcpy(p, "test");
    mapd_insert(&mapd, data_string(p), 42, release);
  }
  assert_true(mapd_at(&mapd, data_string("test")) == 42);
  assert_true(mapd_size(&mapd) == 1);

  mapd_erase(&mapd, data_string("test2"), release);
  assert_true(mapd_size(&mapd) == 1);
  mapd_erase(&mapd, data_string("test"), release);
  assert_true(mapd_size(&mapd) == 0);

  mapd_reserve(&mapd, 32);
  assert_true(mapd.map.elements_capacity == 64);

  for (i = 0; i < 100; i++)
  {
    snprintf(key, sizeof key, "test%d", i);
    p = malloc(strlen(key) + 1);
    strcpy(p, key);
    mapd_insert(&mapd, data_string(p), i, release);
  }
  mapd_clear(&mapd, release);
  mapd_destruct(&mapd, release);
}

int main()
{
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_mapd)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
