#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <cmocka.h>

#include <value.h>

static void test_string(__attribute__((unused)) void **arg)
{
  const char *end, *s;
  string_t s1, s2;

  /* basic */
  s = "test";
  s1 = string_define(s, strlen(s));
  assert_true(string_equal(s1, string("test")));

  /* encoding and decoding */
  s = "\xc2\xa3";
  assert_int_equal(string_utf8_get(s, s + strlen(s), NULL), 0x00a3);
  s = "\xE0\xA0\xB3";
  assert_int_equal(string_utf8_get(s, s + strlen(s), NULL), 0x0833);
  assert_int_equal(string_utf8_get_encoded("\\u1234", NULL), 0x1234);
  assert_int_equal(string_utf8_get_encoded("\\uabcd", NULL), 0xABCD);
  assert_int_equal(string_utf8_get_encoded("\\uABCD", NULL), 0xABCD);
  assert_int_equal(string_utf8_get_encoded("\\ud800\\udc00", NULL), 0x10000);
  assert_int_equal(string_utf8_get_encoded("\\ue000", NULL), 0xe000);

  /* invalid encodings */
  assert_int_equal(string_utf8_get_encoded("xu1234", NULL), 0);
  assert_int_equal(string_utf8_get_encoded("\\x1234", NULL), 0);
  assert_int_equal(string_utf8_get_encoded("\\ux234", NULL), 0);
  assert_int_equal(string_utf8_get_encoded("\\u1x34", NULL), 0);
  assert_int_equal(string_utf8_get_encoded("\\u12x4", NULL), 0);
  assert_int_equal(string_utf8_get_encoded("\\u123x", NULL), 0);
  assert_int_equal(string_utf8_get_encoded("x", NULL), 0);
  assert_int_equal(string_utf8_get_encoded("\\udc00", NULL), 0);
  assert_int_equal(string_utf8_get_encoded("\\ud800\\u0000", NULL), 0);
  assert_int_equal(string_utf8_get_encoded("\\ud800\\ue000", NULL), 0);

  /* valid */
  s1 = string_utf8_decode("\\u0061\\u00a3\\u0833", NULL);
  assert_string_equal(string_base(s1), "a\xc2\xa3\xE0\xA0\xB3");
  string_release(s1);

  /* ascii with utf8 */
  s1 = string_utf8_decode("snowman \xE2\x98\x83", &end);
  assert_string_equal(end, "");
  assert_int_equal(string_size(s1), 11);
  string_release(s1);

  /* ascii with ending with " */
  s1 = string_utf8_decode("snowman \xE2\x98\x83\"", &end);
  assert_string_equal(end, "\"");
  assert_int_equal(string_size(s1), 11);
  assert_int_equal(string_utf8_length(s1), 9);
  string_release(s1);

  /* ascii with invalid escape */
  s1 = string_utf8_decode("test\\x", &end);
  assert_string_equal(end, "\\x");
  assert_int_equal(string_size(s1), 4);
  string_release(s1);

  /* whitespace ending with " */
  s1 = string_utf8_decode("        \"", &end);
  assert_string_equal(end, "\"");
  assert_int_equal(string_size(s1), 8);
  string_release(s1);

  /* ascii with utf8 */
  s1 = string_utf8_decode("control \\\" \\\\ \\/ \\b \\f \\n \\r \\t", &end);
  assert_string_equal(end, "");
  assert_string_equal(string_base(s1), "control \" \\ / \b \f \n \r \t");
  string_release(s1);

  /* invalid utf8 */
  s1 = string_utf8_decode("\xFF\xFF\xFF", NULL);
  assert_false(string_empty(s1));
  assert_int_equal(string_utf8_length(s1), 3);
  string_release(s1);

  /* supplementary plane */
  s1 = string_utf8_decode("aaa\U0001F600bbb", &end);
  assert_string_equal(end, "");
  assert_int_equal(string_size(s1), 10);
  string_release(s1);

  /* escaped supplementary plane */
  s1 = string_utf8_decode("aaa\\ud83d\\ude03bbb", &end);
  assert_string_equal(end, "");
  assert_int_equal(string_size(s1), 10);
  s2 = string_utf8_decode("aaa\U0001f603bbb", &end);
  assert_string_equal(end, "");
  assert_true(string_equal(s1, s2));
  string_release(s1);
  string_release(s2);

  /* equality */
  s1 = string_utf8_decode("testa", NULL);
  s2 = string_utf8_decode("test", NULL);
  assert_false(string_equal(s1, s2));
  string_release(s1);
  string_release(s2);

  s1 = string_utf8_decode("test1", NULL);
  s2 = string_utf8_decode("test2", NULL);
  assert_false(string_equal(s1, s2));
  string_release(s1);
  string_release(s2);

  s1 = string_utf8_decode("tesT1", NULL);
  s2 = string_utf8_decode("tEst1", NULL);
  assert_true(string_equal_case(s1, s2));
  string_release(s1);
  string_release(s2);

  /* clear null */
  s1 = string_null();
  assert_int_equal(string_utf8_length(s1), 0);
  string_release(s1);
}

int main()
{
  const struct CMUnitTest tests[] = {cmocka_unit_test(test_string)};

  return cmocka_run_group_tests(tests, NULL, NULL);
}
