#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <value.h>

static value_t *encapsulate(value_t *object)
{
  value_t *parent = value_object();

  value_object_set_release(parent, string(""), object);
  return parent;
}

static uint64_t ntime(void)
{
  struct timespec tv;
  clock_gettime(CLOCK_MONOTONIC, &tv);
  return (uint64_t) tv.tv_sec * 1000000000ULL + (uint64_t) tv.tv_nsec;
}

int main()
{
  value_t *v, *v2;
  string_t s;
  uint64_t t1, t2;

  v = value_number(42);
  for (int i = 0; i < 1000; i++)
    v = encapsulate(v);
  t1 = ntime();
  s = value_encode(v, 0);
  t2 = ntime();
  value_release(v);
  printf("%d '%s'\n", string_nullp(s), string_base(s));
  printf("time %f\n", ((double) t2 - t1) / 1000000000.);
  string_release(s);

  v = value_undefined();
  s = value_encode(v, 0);
  value_release(v);
  printf("%d '%s'\n", string_nullp(s), string_base(s));
  string_release(s);

  const char p[] = "string, basic: \xE2\x98\x83, supp: \U0001F600, zero: \x00, control \t\t.end";

  printf("string: ");
  fflush(stdout);
  write(1, p, sizeof p);
  printf("\n");

  v2 = value_array();
  value_array_append_release(v2, value_bool(false));
  value_array_append_release(v2, value_bool(true));
  value_array_append_release(v2, value_string(string_define(p, sizeof p - 1)));
  value_array_append_release(v2, value_number(42));
  v = value_object();
  value_object_set_release(v, string("test"), v2);
  t1 = ntime();
  s = value_encode(v, 0);
  t2 = ntime();
  value_release(v);
  printf("%s\n", string_base(s));

  value_t *v3 = value_decode(string_base(s), NULL);
  string_release(s);
  s = value_encode(v3, 0);
  value_release(v3);
  printf("[decoded]\n%s\n", string_base(s));
  string_release(s);

  v3 = value_decode("-234.1231249e90", NULL);
  s = value_encode(v3, 0);
  value_release(v3);
  printf("[decoded]\n%s\n", string_base(s));
  string_release(s);

  v3 = value_decode("[\"a normal string, \\u2603 == \xE2\x98\x83, \\ud83d\\ude00 == \U0001F600\",1, 2, [{\"a\":null} ,{} ], true, 3]", NULL);
  s = value_encode(v3, VALUE_ENCODE_ASCII);
  value_release(v3);
  printf("[decoded]\n%s\n", string_base(s));
  string_release(s);

  v3 = value_decode("[\"a normal string, \\u2603 == \xE2\x98\x83, \\ud83d\\ude00 == \U0001F600\",1, 2, [{\"a\":null} ,{} ], true, 3]", NULL);
  s = value_encode(v3, VALUE_ENCODE_PRETTY);
  value_release(v3);
  printf("[decoded]\n%s\n", string_base(s));
  string_release(s);
}
