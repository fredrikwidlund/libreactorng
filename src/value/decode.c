#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <value.h>

enum state
{
  STATE_VALUE_START,
  STATE_VALUE_END,
  STATE_NULL,
  STATE_BOOL,
  STATE_NUMBER,
  STATE_STRING,
  STATE_ARRAY_FIRST,
  STATE_ARRAY_REST,
  STATE_ARRAY_EMPTY,
  STATE_ARRAY_END,
  STATE_OBJECT_FIRST,
  STATE_OBJECT_MAP,
  STATE_OBJECT_REST,
  STATE_OBJECT_EMPTY,
  STATE_OBJECT_END,
  STATE_REDUCE,
  STATE_ACCEPT,
  STATE_REJECT
};

static void value_decode_whitespace(const char *from, const char **end)
{
  while (*from == ' ' || *from == '\n' || *from == '\r' || *from == '\t')
    from++;
  *end = from;
}

static value_t *value_decode_null(const char *from, const char **end)
{
  if (strncmp(from, "null", 4) == 0)
  {
    *end = from + 4;
    return value_null();
  }
  return value_undefined();
}

static value_t *value_decode_bool(const char *from, const char **end)
{
  if (strncmp(from, "true", 4) == 0)
  {
    *end = from + 4;
    return value_bool(true);
  }
  if (strncmp(from, "false", 5) == 0)
  {
    *end = from + 5;
    return value_bool(false);
  }
  return value_undefined();
}

static bool value_skip_digits(const char *from, const char **end)
{
  const char *p = from;

  if (!isdigit(*p))
    return false;
  p++;
  while (isdigit(*p))
    p++;
  *end = p;
  return true;
}

static value_t *value_decode_number(const char *from, const char **end)
{
  const char *p = from;
  long double n;

  if (*p == '-')
    p++;
  if (*p == '0' && isdigit(p[1]))
    return value_undefined();
  if (!value_skip_digits(p, &p))
    return value_undefined();
  if (*p == '.')
  {
    p++;
    if (!value_skip_digits(p, &p))
      return value_undefined();
  }
  if (*p == 'e' || *p == 'E')
  {
    p++;
    if (*p == '-' || *p == '+')
      p++;
    if (*p == '0' && isdigit(p[1]))
      return value_undefined();
    if (!value_skip_digits(p, &p))
      return value_undefined();
  }
  errno = 0;
  n = strtold(from, (char **) end);
  return *end == p && errno == 0 ? value_number(n) : value_undefined();
}

static value_t *value_decode_string(const char *from, const char **end)
{
  const char *p = from;
  string_t s;

  // always called from a string starting with '"'
  p++;
  s = string_utf8_decode(p, &p);
  if (*p != '"')
  {
    string_release(s);
    return value_undefined();
  }
  p++;
  *end = p;
  return value_string_release(s);
}

static void value_decode_release(void *arg)
{
  value_t *value = *(value_t **) arg;

  value_release(value);
}

value_t *value_decode(const char *from, const char **end)
{
  const char *p = from;
  enum state state, state_old;
  vector_t stack;
  value_t *value, *parent, *key;

  vector_construct(&stack, sizeof value);
  value = value_undefined();
  state = STATE_VALUE_START;
  while (1)
  {
    value_decode_whitespace(p, &p);
    state_old = state;
    state = STATE_REJECT;
    switch (state_old)
    {
    case STATE_VALUE_START:
      switch (*p)
      {
      case 'n':
        state = STATE_NULL;
        break;
      case 't':
      case 'f':
        state = STATE_BOOL;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case '-':
        state = STATE_NUMBER;
        break;
      case '"':
        state = STATE_STRING;
        break;
      case '[':
        state = STATE_ARRAY_FIRST;
        break;
      case ']':
        state = STATE_ARRAY_EMPTY;
        break;
      case '{':
        state = STATE_OBJECT_FIRST;
        break;
      case '}':
        state = STATE_OBJECT_EMPTY;
      };
      break;
    case STATE_VALUE_END:
      if (value_is_undefined(value))
        state = STATE_REJECT;
      else if (vector_empty(&stack))
        state = STATE_ACCEPT;
      else
        state = STATE_REDUCE;
      break;
    case STATE_NULL:
      value = value_decode_null(p, &p);
      state = STATE_VALUE_END;
      break;
    case STATE_BOOL:
      value = value_decode_bool(p, &p);
      state = STATE_VALUE_END;
      break;
    case STATE_NUMBER:
      value = value_decode_number(p, &p);
      state = STATE_VALUE_END;
      break;
    case STATE_STRING:
      value = value_decode_string(p, &p);
      state = STATE_VALUE_END;
      break;
    case STATE_ARRAY_FIRST:
      p++;
      parent = value_array();
      vector_push_back(&stack, &parent);
      state = STATE_VALUE_START;
      break;
    case STATE_ARRAY_REST:
      if (*p == ',')
      {
        p++;
        state = STATE_VALUE_START;
      }
      else if (*p == ']')
        state = STATE_ARRAY_END;
      break;
    case STATE_ARRAY_EMPTY:
      if (vector_empty(&stack))
        break;
      parent = *(value_t **) vector_back(&stack);
      if (value_is_array(parent) && value_array_length(parent) == 0)
        state = STATE_ARRAY_END;
      break;
    case STATE_ARRAY_END:
      parent = *(value_t **) vector_back(&stack);
      /* always array */
      p++;
      vector_pop_back(&stack, NULL);
      value = parent;
      state = STATE_VALUE_END;
      break;
    case STATE_OBJECT_FIRST:
      p++;
      parent = value_object();
      vector_push_back(&stack, &parent);
      state = STATE_VALUE_START;
      break;
    case STATE_OBJECT_MAP:
      if (*p == ':')
      {
        p++;
        state = STATE_VALUE_START;
      }
      break;
    case STATE_OBJECT_REST:
      if (*p == ',')
      {
        p++;
        state = STATE_VALUE_START;
      }
      else if (*p == '}')
        state = STATE_OBJECT_END;
      break;
    case STATE_OBJECT_EMPTY:
      if (vector_empty(&stack))
        break;
      parent = *(value_t **) vector_back(&stack);
      if (value_is_object(parent) && value_object_size(parent) == 0)
        state = STATE_OBJECT_END;
      break;
    case STATE_OBJECT_END:
      parent = *(value_t **) vector_back(&stack);
      /* always object */
      p++;
      vector_pop_back(&stack, NULL);
      value = parent;
      state = STATE_VALUE_END;
      break;
    case STATE_REDUCE:
      parent = *(value_t **) vector_back(&stack);
      if (value_is_array(parent))
      {
        value_array_append_release(parent, value);
        value = value_undefined();
        state = STATE_ARRAY_REST;
      }
      else if (value_is_object(parent) && value_is_string(value))
      {
        vector_push_back(&stack, &value);
        value = value_undefined();
        state = STATE_OBJECT_MAP;
      }
      else if (value_is_string(parent))
      {
        key = parent;
        vector_pop_back(&stack, NULL);
        parent = *(value_t **) vector_back(&stack);
        value_object_set_release(parent, value_string_get(key), value);
        value_release(key);
        value = value_undefined();
        state = STATE_OBJECT_REST;
      }
      break;
    case STATE_ACCEPT:
      if (end)
        *end = p;
      vector_destruct(&stack, value_decode_release);
      return value;
      break;
    default:
    case STATE_REJECT:
      vector_destruct(&stack, value_decode_release);
      value_release(value);
      return value_undefined();
    }
  }
}
