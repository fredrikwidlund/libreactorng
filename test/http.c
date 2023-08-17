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

struct test
{
  const char *request;
  int result;
  int remaining;
};

struct test request_tests[] =
{
  {"", 0, 0},
  {"GET / HTTP/1.0\r\nA: B\r\n\r\n", 1, 0},
  {"GET / HTTP/1.0\r\n", 0, 16},
  {"GET / XXXX/1.0\r\n", -1, 16},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "\r\n", 1, 0},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "0\r\n"
   "\r\n", 1, 0},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "1\r\n"
   "a\r\n"
   "0\r\n"
   "\r\n",  1, 0},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "\t 0 \r\n"
   "\r\n", 1, 0},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "\t 0; xxxxx   xxxx \r\n"
   "\r\n", 1, 0},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "\t 0; xxxxx   xxxx \n"
   "\r\n", -1, 85},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "\t 0  xxxxx   xxxx \r\n"
   "\r\n", -1, 86},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "FFFFFFFFFFFFFFFFFFFFF\r\n"
   "\r\n", -1, 89},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "\t0\r \n"
   "\r\n", -1, 71},
  {"POST / HTTP/1.1\r\n"
    "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "0", 0, 65},
  {"POST / HTTP/1.1\r\n"
    "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "0\r\n", 0, 67},
  {"POST / HTTP/1.1\r\n"
    "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "FF\r\n"
   "\r\n"
   , 0, 70},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "\r\n"
   "\r\n", -1, 66},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Content-Length: 0\r\n"
   "\r\n", 1, 0},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Content-Length: 999\r\n"
   "\r\n", 0, 57},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: chunked\r\n"
   "Content-Length: 0\r\n"
   "\r\n", -1, 83},
  {"POST / HTTP/1.1\r\n"
   "Host: localhost\r\n"
   "Transfer-Encoding: XXXX\r\n"
   "\r\n", -1, 61},
};

static void test_read_request(__attribute__((unused)) void **arg)
{
  stream_t s;
  string_t method, target;
  data_t body;
  http_field_t fields[16];
  size_t fields_count;
  size_t i;
  int n;

  for (i = 0; i < sizeof request_tests / sizeof *request_tests; i++)
  {
    stream_construct(&s, NULL, NULL);
    buffer_append(&s.input, string(request_tests[i].request));
    fields_count = 16;
    n = http_read_request(&s, &method, &target, &body, fields, &fields_count);
    assert_int_equal(n, request_tests[i].result);
    assert_int_equal(data_size(stream_read(&s)), request_tests[i].remaining);
    stream_destruct(&s);
  }
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
