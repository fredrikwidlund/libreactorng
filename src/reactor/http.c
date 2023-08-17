#include <stdlib.h>
#include <string.h>
#include <reactor.h>
#include "picohttpparser/picohttpparser.h"

static size_t http_u32_len(uint32_t n)
{
  static const uint32_t pow_10[] = {0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};
  uint32_t t;

  t = (32 - __builtin_clz(n | 1)) * 1233 >> 12;
  return t - (n < pow_10[t]) + 1;
}

static void http_u32_sprint(uint32_t n, char *string)
{
  static const char digits[] =
      "0001020304050607080910111213141516171819"
      "2021222324252627282930313233343536373839"
      "4041424344454647484950515253545556575859"
      "6061626364656667686970717273747576777879"
      "8081828384858687888990919293949596979899";
  size_t i;

  while (n >= 100)
  {
    i = (n % 100) << 1;
    n /= 100;
    *--string = digits[i + 1];
    *--string = digits[i];
  }

  if (n < 10)
  {
    *--string = n + '0';
  }
  else
  {
    i = n << 1;
    *--string = digits[i + 1];
    *--string = digits[i];
  }
}

static void http_move(void **pointer, size_t size)
{
  *(uint8_t **) pointer += size;
}
static void http_push_data(void **pointer, data_t data)
{
  memcpy(*pointer, data_base(data), data_size(data));
  http_move(pointer, data_size(data));
}

static void http_push_byte(void **pointer, uint8_t byte)
{
  **((uint8_t **) pointer) = byte;
  http_move(pointer, 1);
}

static void http_push_field(void **pointer, http_field_t field)
{
  http_push_data(pointer, field.name);
  http_push_byte(pointer, ':');
  http_push_byte(pointer, ' ');
  http_push_data(pointer, field.value);
  http_push_byte(pointer, '\r');
  http_push_byte(pointer, '\n');
}

http_field_t http_field_define(string_t name, string_t value)
{
  return (http_field_t) {.name = name, .value = value};
}

int http_read_request(stream_t *stream, string_t *method, string_t *target, http_field_t *fields, size_t *fields_count)
{
  data_t data;
  int n, minor_version;

  data = stream_read(stream);
  if (data_empty(data))
    return 0;

  n = phr_parse_request(data_base(data), data_size(data),
                        (const char **) &method->iov.iov_base, &method->iov.iov_len,
                        (const char **) &target->iov.iov_base, &target->iov.iov_len,
                        &minor_version,
                        (struct phr_header *) fields, fields_count, 0);
  asm volatile("": : :"memory");
  if (n <= 0)
    return n == -1 ? -1 : 0;
  stream_consume(stream, n);
  return 1;
}

static void http_write_response_basic(stream_t *stream, string_t status, string_t date, string_t type, data_t body)
{
  char length_storage[16];
  size_t length_size, size;
  string_t length;
  void *p;

  length_size = http_u32_len(data_size(body));
  http_u32_sprint(data_size(body), length_storage + length_size);
  length = data(length_storage, length_size);

  size = 9 + string_size(status) + 2 + 11 + 37 + 16 + string_size(type) + 18 + string_size(length) + 2 + data_size(body);
  p = stream_allocate(stream, size);
  http_push_data(&p, string("HTTP/1.1 "));
  http_push_data(&p, status);
  http_push_data(&p, string("\r\n"));
  http_push_field(&p, http_field_define(string("Server"), string("*")));
  http_push_field(&p, http_field_define(string("Date"), date));
  http_push_field(&p, http_field_define(string("Content-Type"), type));
  http_push_field(&p, http_field_define(string("Content-Length"), length));
  http_push_data(&p, string("\r\n"));
  http_push_data(&p, body);
}

static void http_write_response_extended(stream_t *stream, string_t status, string_t date, string_t type, data_t body,
                                         http_field_t *fields, size_t fields_count)
{
  char length_storage[16];
  size_t length_size, size, i;
  string_t length;
  void *p;

  length_size = http_u32_len(data_size(body));
  http_u32_sprint(data_size(body), length_storage + length_size);
  length = data(length_storage, length_size);

  size = 9 + string_size(status) + 2 + 11 + 37 + 16 + string_size(type) + 18 + string_size(length) + 2 + data_size(body);
  for (i = 0; i < fields_count; i++)
    size += string_size(fields[i].name) + 2 + string_size(fields[i].value) + 2;

  p = stream_allocate(stream, size);
  http_push_data(&p, string("HTTP/1.1 "));
  http_push_data(&p, status);
  http_push_data(&p, string("\r\n"));
  http_push_field(&p, http_field_define(string("Server"), string("*")));
  http_push_field(&p, http_field_define(string("Date"), date));
  http_push_field(&p, http_field_define(string("Content-Type"), type));
  http_push_field(&p, http_field_define(string("Content-Length"), length));
  for (i = 0; i < fields_count; i++)
    http_push_field(&p, fields[i]);
  http_push_data(&p, string("\r\n"));
  http_push_data(&p, body);
}

inline __attribute__((always_inline, flatten))
void http_write_response(stream_t *stream, string_t status, string_t date, string_t type, data_t body, http_field_t *field, size_t field_count)
{
  if (field_count == 0)
    http_write_response_basic(stream, status, date, type, body);
  else
    http_write_response_extended(stream, status, date, type, body, field, field_count);
}
