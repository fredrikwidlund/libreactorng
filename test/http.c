#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include <errno.h>
#include <limits.h>
#include <sys/socket.h>
#include <cmocka.h>

#include "reactor.h"

static void test_read_request(__attribute__((unused)) void **arg)
{
  stream_t s;
  string_t method, target;
  http_field_t fields[16];
  size_t fields_count = 16;
  int n;

  stream_construct(&s, NULL, NULL);

  n = http_read_request(&s, &method, &target, fields, &fields_count);
  assert_int_equal(n, 0);

  buffer_append(&s.input, string("GET / HTTP/1.0\r\n\r\n"));
  n = http_read_request(&s, &method, &target, fields, &fields_count);
  assert_int_equal(n, 1);

  buffer_clear(&s.input);
  s.input_consumed = 0;

  buffer_append(&s.input, string("GET / HTTP/1.0\r\n"));
  n = http_read_request(&s, &method, &target, fields, &fields_count);
  assert_int_equal(n, 0);

  buffer_clear(&s.input);
  s.input_consumed = 0;

  buffer_append(&s.input, string("GET / XXXX/1.0\r\n"));
  n = http_read_request(&s, &method, &target, fields, &fields_count);
  assert_int_equal(n, -1);

  stream_destruct(&s);
}

static void test_write_response(__attribute__((unused)) void **arg)
{
  stream_t s;
  char blob[1024] = {0};

  stream_construct(&s, NULL, NULL);

  http_write_response(&s, string("200 OK"), string("Wed, 16 Aug 2023 10:29:23 GMT"), string("text/plain"), string("Hello"), NULL, 0);
  assert_true(string_equal(string(
                             "HTTP/1.1 200 OK\r\n"
                             "Server: *\r\n"
                             "Date: Wed, 16 Aug 2023 10:29:23 GMT\r\n"
                             "Content-Type: text/plain\r\n"
                             "Content-Length: 5\r\n"
                             "\r\n"
                             "Hello"
                             ), buffer_data(&s.output_waiting)));
  buffer_clear(&s.output_waiting);

  http_write_response(&s, string("200 OK"), string("Wed, 16 Aug 2023 10:29:23 GMT"), string("text/plain"), string("Hello, again"),
                      (http_field_t []) {http_field_define(string("Cookie"), string("Test"))}, 1);
  assert_true(string_equal(string(
                             "HTTP/1.1 200 OK\r\n"
                             "Server: *\r\n"
                             "Date: Wed, 16 Aug 2023 10:29:23 GMT\r\n"
                             "Content-Type: text/plain\r\n"
                             "Content-Length: 12\r\n"
                             "Cookie: Test\r\n"
                             "\r\n"
                             "Hello, again"
                             ), buffer_data(&s.output_waiting)));
  buffer_clear(&s.output_waiting);

  http_write_response(&s, string("200 OK"), string("Wed, 16 Aug 2023 10:29:23 GMT"), string("text/plain"), data(blob, sizeof blob), NULL, 0);
  assert_int_equal(buffer_size(&s.output_waiting), 1139);
  buffer_clear(&s.output_waiting);

  stream_destruct(&s);
}

int main()
{
  const struct CMUnitTest tests[] =
    {
      cmocka_unit_test(test_read_request),
      cmocka_unit_test(test_write_response)
    };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
