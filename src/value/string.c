#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <value.h>

static const char *string_null_chars = "";

/* constructor/destructor */

string_t string(const char *text)
{
  return data_string(text);
}

string_t string_define(const void *base, size_t size)
{
  return data_define(base, size);
}

string_t string_null(void)
{
  return data_define(string_null_chars, 0);
}

string_t string_copy(const string_t string)
{
  return string_nullp(string) ? string_null() : data_copyz(string);
}

void string_release(const string_t string)
{
  if (!string_nullp(string))
    data_release(string);
}

/* capacity */

size_t string_size(const string_t string)
{
  return data_size(string);
}

bool string_nullp(string_t string)
{
  return string_base(string) == string_null_chars;
}

bool string_empty(const string_t string)
{
  return data_empty(string);
}

/* element access */

char *string_base(const string_t string)
{
  return data_base(string);
}

bool string_equal(const string_t a, const string_t b)
{
  return data_equal(a, b);
}

bool string_equal_case(const string_t a, const string_t b)
{
  return data_equal_case(a, b);
}

/* utf8 */

/* utf8 decoder from http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ */

enum
{
  STATE_ACCEPT = 0,
  STATE_REJECT = 12
};

static uint32_t string_utf8_decode_code(uint32_t *state, uint32_t *codep, uint8_t byte)
{
  static const uint8_t utf8d[] = {
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
      1,  1,  1,  1,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  7,  7,  7,  7,  7,  7,  7,  7,
      7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  2,  2,
      2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,
      10, 3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  4,  3,  3,  11, 6,  6,  6,  5,  8,  8,  8,  8,  8,  8,  8,
      8,  8,  8,  8,  0,  12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
      12, 0,  12, 12, 12, 12, 12, 0,  12, 0,  12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, 12, 12, 12, 12,
      12, 12, 12, 24, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, 12, 12, 12, 12, 12, 36,
      12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, 12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12};
  uint32_t type = utf8d[byte];

  *codep = (*state != STATE_ACCEPT) ? (byte & 0x3fu) | (*codep << 6) : (0xff >> type) & (byte);

  *state = utf8d[256 + *state + type];
  return *state;
}

static bool string_utf8_regular(uint8_t c)
{
  if (c == '"')
    return false;
  if (c == '\\')
    return false;
  if (c < 0x20)
    return false;
  if (c >= 0x80)
    return false;
  return true;
}

static uint16_t string_utf8_get_encoded_basic(const char *from, const char **end)
{
  const uint8_t xdigit[] = {['0'] = 0, 1,  2,  3,  4,  5,          6,  7,  8,  9,  ['a'] = 10,
                            11,        12, 13, 14, 15, ['A'] = 10, 11, 12, 13, 14, 15};
  const char *p = from;
  uint32_t code = 0;

  if (p[0] == '\\' && p[1] == 'u' && isxdigit(p[2]) && isxdigit(p[3]) && isxdigit(p[4]) && isxdigit(p[5]))
  {
    code = xdigit[(int) p[2]] << 12 | xdigit[(int) p[3]] << 8 | xdigit[(int) p[4]] << 4 | xdigit[(int) p[5]];
    p += 6;
  }
  *end = p;
  return code;
}

size_t string_utf8_length(const string_t string)
{
  const char *from = string_base(string), *to = from + string_size(string);
  size_t length = 0;

  while (from < to)
  {
    (void) string_utf8_get(from, to, &from);
    length++;
  }
  return length;
}

uint32_t string_utf8_get(const char *from, const char *to, const char **end)
{
  uint32_t state = STATE_ACCEPT, code = 0;

  if (end)
    *end = from;
  while (from < to)
  {
    string_utf8_decode_code(&state, &code, *from);
    from++;
    if (state == STATE_ACCEPT)
      break;
    if (state == STATE_REJECT || from == to)
    {
      code = 0xFFFD;
      break;
    }
  }
  if (end)
    *end = from;
  return code;
}

uint32_t string_utf8_get_encoded(const char *from, const char **end)
{
  const char *p, *p_saved;
  uint16_t w1, w2;
  uint32_t code = 0;

  p_saved = p = from;
  w1 = string_utf8_get_encoded_basic(p, &p);
  if (p > p_saved)
  {
    if (w1 >= 0xdc00 && w1 < 0xe000)
      p = p_saved;
    else if (w1 >= 0xd800 && w1 < 0xdc00)
    {
      w2 = string_utf8_get_encoded_basic(p, &p);
      if (w2 >= 0xdc00 && w2 < 0xe000)
        code = 0x10000 | ((w1 & 0x3ff) << 10) | (w2 & 0x3ff);
      else
        p = p_saved;
    }
    else
      code = w1;
  }

  if (end)
    *end = p;
  return code;
}

void string_utf8_put(buffer_t *buffer, uint32_t code)
{
  if (code < 0x80)
    buffer_append(buffer, data_define((uint8_t[]) {code}, 1));
  else if (code < 0x800)
    buffer_append(buffer, data_define((uint8_t[]) {0b11000000 | (code >> 6), 0b10000000 | (code & 0x3f)}, 2));
  else if (code < 0x10000)
    buffer_append(buffer, data_define((uint8_t[]) {0b11100000 | (code >> 12), 0b10000000 | ((code >> 6) & 0x3f),
                                                   0b10000000 | (code & 0x3f)},
                                      3));
  else
    buffer_append(buffer, data_define((uint8_t[]) {0b11110000 | (code >> 18), 0b10000000 | ((code >> 12) & 0x3f),
                                                   0b10000000 | ((code >> 6) & 0x3f), 0b10000000 | (code & 0x3f)},
                                      4));
}

void string_utf8_put_encoded_basic(buffer_t *buffer, uint16_t code)
{
  static const char map[16] = "0123456789abcdef";
  char s[6] = {'\\', 'u', map[code >> 12], map[(code >> 8) & 0x0f], map[(code >> 4) & 0x0f], map[code & 0x0f]};

  buffer_append(buffer, data_define(s, sizeof s));
}

void string_utf8_put_encoded(buffer_t *buffer, uint32_t code)
{
  if (code >= 0x10000)
  {
    code -= 0x10000;
    string_utf8_put_encoded_basic(buffer, 0xd800 | (code >> 10));
    string_utf8_put_encoded_basic(buffer, 0xdc00 | (code & 0x3ff));
  }
  else
    string_utf8_put_encoded_basic(buffer, code);
}

bool string_utf8_encode(buffer_t *buffer, const string_t utf8_string, bool ascii)
{
  const char *p_old, *p = string_base(utf8_string), *p_end = p + string_size(utf8_string);
  uint32_t code;

  while (1)
  {
    if (p == p_end)
      return true;

    p_old = p;
    if (string_utf8_regular(*p))
    {
      p++;
      while (p < p_end && string_utf8_regular(*p))
        p++;
      buffer_append(buffer, data_define(p_old, p - p_old));
    }
    else if ((unsigned char) *p >= 0x80)
    {
      code = string_utf8_get(p, p_end, &p);
      if (ascii)
        string_utf8_put_encoded(buffer, code);
      else
        string_utf8_put(buffer, code);
    }
    else
    {
      switch (*p)
      {
      case '"':
        buffer_append(buffer, string("\\\""));
        break;
      case '\\':
        buffer_append(buffer, string("\\\\"));
        break;
      case '\f':
        buffer_append(buffer, string("\\f"));
        break;
      case '\n':
        buffer_append(buffer, string("\\n"));
        break;
      case '\r':
        buffer_append(buffer, string("\\r"));
        break;
      case '\t':
        buffer_append(buffer, string("\\t"));
        break;
      default:
        string_utf8_put_encoded_basic(buffer, *p);
        break;
      }
      p++;
    }
  }
}

string_t string_utf8_decode(const char *from, const char **end)
{
  buffer_t buffer;
  const char *p, *p_old;
  uint32_t code;

  buffer_construct(&buffer);
  p = from;
  while (1)
  {
    p_old = p;
    if (string_utf8_regular(*p))
    {
      p++;
      while (string_utf8_regular(*p))
        p++;
      buffer_append(&buffer, data_define(p_old, p - p_old));
    }
    else if ((unsigned char) *p >= 0x80)
    {
      code = string_utf8_get(p, p + 4, &p);
      string_utf8_put(&buffer, code);
    }
    else if (*p == '\\')
    {
      switch (p[1])
      {
      case '"':
        buffer_append(&buffer, string("\""));
        p += 2;
        break;
      case '\\':
        buffer_append(&buffer, string("\\"));
        p += 2;
        break;
      case '/':
        buffer_append(&buffer, string("/"));
        p += 2;
        break;
      case 'b':
        buffer_append(&buffer, string("\b"));
        p += 2;
        break;
      case 'f':
        buffer_append(&buffer, string("\f"));
        p += 2;
        break;
      case 'n':
        buffer_append(&buffer, string("\n"));
        p += 2;
        break;
      case 'r':
        buffer_append(&buffer, string("\r"));
        p += 2;
        break;
      case 't':
        buffer_append(&buffer, string("\t"));
        p += 2;
        break;
      case 'u':
        string_utf8_put(&buffer, string_utf8_get_encoded(p, &p));
        break;
      }
    }
    if (p == p_old)
      break;
  }

  if (end)
    *end = p;
  if (buffer_empty(&buffer))
  {
    buffer_destruct(&buffer);
    return string_null();
  }
  buffer_append(&buffer, data_define("", 1));
  buffer_compact(&buffer);
  buffer_resize(&buffer, buffer_size(&buffer) - 1);
  return buffer_data(&buffer);
}
