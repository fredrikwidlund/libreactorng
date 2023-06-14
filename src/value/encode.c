#include <value.h>

typedef struct context context_t;

struct context
{
  const value_t *value;
  value_t *keys;
  size_t index;
};

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
  STATE_OBJECT_FIRST,
  STATE_OBJECT_REST,
  STATE_ACCEPT,
  STATE_REJECT
};

static void value_encode_release_context(void *arg)
{
  context_t *context = arg;

  value_release(context->keys);
}

static void value_buffer_encode_number(buffer_t *buffer, long double number)
{
  char str[64];

  snprintf(str, sizeof str, "%.32Lg", number);
  buffer_append(buffer, data_string(str));
}

static void value_buffer_encode_string(buffer_t *buffer, const string_t s, bool ascii)
{
  buffer_append(buffer, string("\""));
  string_utf8_encode(buffer, s, ascii);
  buffer_append(buffer, string("\""));
}

__attribute__((flatten)) bool value_buffer_encode(buffer_t *buffer, const value_t *value,
                                                  value_encode_options_t options)
{
  context_t *context;
  enum state state, state_old;
  vector_t stack;
  string_t key;
  bool pretty = options & VALUE_ENCODE_PRETTY;
  bool ascii = options & VALUE_ENCODE_ASCII;

  vector_construct(&stack, sizeof *context);
  state = STATE_VALUE_START;
  while (1)
  {
    state_old = state;
    state = STATE_REJECT;
    switch (state_old)
    {
    case STATE_VALUE_START:
      if (value_nullp(value))
        state = STATE_NULL;
      else if (value_boolp(value))
        state = STATE_BOOL;
      else if (value_numberp(value))
        state = STATE_NUMBER;
      else if (value_stringp(value))
        state = STATE_STRING;
      else if (value_objectp(value))
        state = STATE_OBJECT_FIRST;
      else if (value_arrayp(value))
        state = STATE_ARRAY_FIRST;
      break;
    case STATE_VALUE_END:
      if (vector_empty(&stack))
        state = STATE_ACCEPT;
      else
      {
        context = vector_back(&stack);
        state = context->keys ? state = STATE_OBJECT_REST : STATE_ARRAY_REST;
      }
      break;
    case STATE_NULL:
      buffer_append(buffer, data_string("null"));
      state = STATE_VALUE_END;
      break;
    case STATE_BOOL:
      buffer_append(buffer, value_bool_get(value) ? data_string("true") : data_string("false"));
      state = STATE_VALUE_END;
      break;
    case STATE_NUMBER:
      value_buffer_encode_number(buffer, value_number_get(value));
      state = STATE_VALUE_END;
      break;
    case STATE_STRING:
      value_buffer_encode_string(buffer, value_string_get(value), ascii);
      state = STATE_VALUE_END;
      break;
    case STATE_ARRAY_FIRST:
      vector_push_back(&stack, (context_t[]) {{.value = value}});
      buffer_append(buffer, string("["));
      state = STATE_ARRAY_REST;
      break;
    case STATE_ARRAY_REST:
      context = vector_back(&stack);
      if (context->index == value_array_length(context->value))
      {
        vector_pop_back(&stack, value_encode_release_context);
        if (pretty && context->index)
        {
          buffer_append(buffer, data_string("\n"));
          buffer_insert_fill(buffer, buffer_size(buffer), vector_size(&stack) * 2, data_string(" "));
          buffer_append(buffer, data_string("]"));
        }
        else
          buffer_append(buffer, string("]"));
        state = STATE_VALUE_END;
        break;
      }
      if (context->index)
        buffer_append(buffer, data_string(","));
      if (pretty)
      {
        buffer_append(buffer, data_string("\n"));
        buffer_insert_fill(buffer, buffer_size(buffer), vector_size(&stack) * 2, data_string(" "));
      }
      value = value_array_get(context->value, context->index);
      context->index++;
      state = STATE_VALUE_START;
      break;
    case STATE_OBJECT_FIRST:
      vector_push_back(&stack, (context_t[]) {{.value = value, .keys = value_object_keys(value)}});
      buffer_append(buffer, string("{"));
      state = STATE_OBJECT_REST;
      break;
    case STATE_OBJECT_REST:
      context = vector_back(&stack);
      if (context->index == value_array_length(context->keys))
      {
        vector_pop_back(&stack, value_encode_release_context);
        if (pretty && context->index)
        {
          buffer_append(buffer, data_string("\n"));
          buffer_insert_fill(buffer, buffer_size(buffer), vector_size(&stack) * 2, data_string(" "));
          buffer_append(buffer, data_string("}"));
        }
        else
          buffer_append(buffer, string("}"));
        state = STATE_VALUE_END;
        break;
      }

      if (context->index)
        buffer_append(buffer, string(","));

      if (pretty)
      {
        buffer_append(buffer, data_string("\n"));
        buffer_insert_fill(buffer, buffer_size(buffer), vector_size(&stack) * 2, data_string(" "));
        key = value_string_get(value_array_get(context->keys, context->index));
        value_buffer_encode_string(buffer, key, ascii);
        buffer_append(buffer, string(": "));
      }
      else
      {
        key = value_string_get(value_array_get(context->keys, context->index));
        value_buffer_encode_string(buffer, key, ascii);
        buffer_append(buffer, string(":"));
      }
      context->index++;
      value = value_object_get(context->value, key);
      state = STATE_VALUE_START;
      break;
    case STATE_ACCEPT:
      vector_destruct(&stack, value_encode_release_context);
      return true;
    default:
    case STATE_REJECT:
      vector_destruct(&stack, value_encode_release_context);
      return false;
    }
  }
}

string_t value_encode(const value_t *value, value_encode_options_t options)
{
  buffer_t buffer;
  bool status;

  buffer_construct(&buffer);
  status = value_buffer_encode(&buffer, value, options);
  if (!status)
  {
    buffer_destruct(&buffer);
    return string_null();
  }
  buffer_append(&buffer, data_define((char[]) {0}, 1));
  buffer_compact(&buffer);
  buffer_resize(&buffer, buffer_size(&buffer) - 1);
  return buffer_data(&buffer);
}
