#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>
#include <reactor.h>

static char *accepts[] = {"42",     "-1e2",
                         "[-1e0]", "1.0E+2",
                         "1.0E-9", "[null, \t\n\r true, false, 2.13, \"test\", [], [1], {}, {\"a\":1, \"b\":2}]"};

static char *rejects[] = {
    "-a",  "01",     "1.",   "1e09", "1e99999999", "0x10", "1e",           "nul",      "tru",         "fals",
    "[",   "]",      "+1",   "\"",   "{",          "}",    "{\"a\"",       "{\"a\":}", "{\"a\":42,}", "{null:42}",
    "[42", "[1,,2]", "[1,]", "[}",   "[null}",     "{]",   "{\"a\":null]", "[null",    "[nullnull",
};

static bool test(const char *input)
{
  value_t *v;
  string_t s;
  const char *eof;
  bool result;

  v = value_decode(input, &eof);
  if (value_is_undefined(v) || *eof)
  {
    value_release(v);
    return false;
  }
  s = value_encode(v, 0);
  value_release(v);
  result = !string_empty(s);
  string_release(s);
  return result;
}

static void test_decode(__attribute__((unused)) void **arg)
{
  size_t i;
  value_t *v;

  for (i = 0; i < sizeof accepts / sizeof *accepts; i++)
    assert_true(test(accepts[i]));
  for (i = 0; i < sizeof rejects / sizeof *rejects; i++)
    assert_false(test(rejects[i]));

  v = value_decode("42", NULL);
  assert_false(value_is_undefined(v));
  value_release(v);
}

static void test_encode(__attribute__((unused)) void **arg)
{
  assert_true(string_empty(value_encode(value_undefined(), 0)));
}

static void test_encode_pretty(__attribute__((unused)) void **arg)
{
  value_t *v;
  string_t s;
  char *in = "{\"a\":[[[], [0], [1, 2]]], \"b\":{}}";
  char *out = "{\n"
              "  \"a\": [\n"
              "    [\n"
              "      [],\n"
              "      [\n"
              "        0\n"
              "      ],\n"
              "      [\n"
              "        1,\n"
              "        2\n"
              "      ]\n"
              "    ]\n"
              "  ],\n"
              "  \"b\": {}\n"
              "}";

  v = value_decode(in, NULL);
  s = value_encode(v, VALUE_ENCODE_PRETTY);
  assert_string_equal(string_base(s), out);
  value_release(v);
  string_release(s);
}

int main()
{
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_decode), cmocka_unit_test(test_encode),
                                     cmocka_unit_test(test_encode_pretty)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
