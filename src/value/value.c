#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <value.h>

typedef struct value_header value_header_t;

struct value_header
{
  _Atomic size_t ref;
  const value_table_t *table;
  char value[];
};

static value_header_t *value_header(const value_t *value)
{
  return (value_header_t *) ((char *) value - offsetof(value_header_t, value));
}

void *value_create(const void *base, size_t size, const value_table_t *table)
{
  value_header_t *header;

  header = malloc(sizeof *header + size);
  *header = (value_header_t) {.ref = 1, .table = table};
  if (base)
    memcpy(header->value, base, size);
  return header->value;
}

size_t value_ref(const value_t *value)
{
  return value ? value_header(value)->ref : 0;
}

void value_hold(value_t *value)
{
  value_header_t *header = value_header(value);

  if (value && header->ref)
    header->ref++;
}

void value_release(value_t *value)
{
  value_header_t *header = value_header(value);

  if (value && header->ref && __sync_sub_and_fetch(&header->ref, 1) == 0)
  {
    if (header->table && header->table->destroy)
      header->table->destroy(value);
    free(header);
  }
}

uintptr_t value_user(const value_t *value)
{
  value_header_t *header = value_header(value);

  return value && header->table ? header->table->user : 0;
}

const char *value_type(const value_t *value)
{
  return value ? (const char *) value_user(value) : VALUE_UNDEFINED;
}

/* undefined */

const char *const VALUE_UNDEFINED = "undefined";

value_t *value_undefined(void)
{
  return NULL;
}

bool value_undefinedp(const value_t *value)
{
  return value_type(value) == VALUE_UNDEFINED;
}

/* null */

const char *const VALUE_NULL = "null";

value_t *value_null(void)
{
  static value_table_t value_null_table = {.user = (uintptr_t) VALUE_NULL};
  static value_header_t value_null_header = {.table = &value_null_table};

  return (value_t *) value_null_header.value;
}

bool value_nullp(const value_t *value)
{
  return value_type(value) == VALUE_NULL;
}

/* bool */

const char *const VALUE_BOOL = "bool";

value_t *value_bool(bool boolean)
{
  static value_table_t value_bool_table = {.user = (uintptr_t) VALUE_BOOL};

  return value_create(&boolean, sizeof boolean, &value_bool_table);
}

bool value_boolp(const value_t *boolean)
{
  return value_type(boolean) == VALUE_BOOL;
}

bool value_bool_get(const value_t *boolean)
{
  return value_boolp(boolean) ? *(bool *) boolean : false;
}

/* number */

const char *const VALUE_NUMBER = "number";

value_t *value_number(long double number)
{
  static value_table_t value_number_table = {.user = (uintptr_t) VALUE_NUMBER};

  return value_create(&number, sizeof number, &value_number_table);
}

bool value_numberp(const value_t *number)
{
  return value_type(number) == VALUE_NUMBER;
}

long double value_number_get(const value_t *number)
{
  return value_numberp(number) ? *(long double *) number : 0;
}

/* string */

const char *const VALUE_STRING = "string";

static void value_string_release_string(void *arg)
{
  string_release(*(string_t *) arg);
}

value_t *value_string(const string_t string)
{
  static value_table_t value_string_table = {.user = (uintptr_t) VALUE_STRING,
                                                   .destroy = value_string_release_string};
  string_t copy = data_copyz(string);

  return value_create(&copy, sizeof copy, &value_string_table);
}

value_t *value_string_constant(const string_t string)
{
  static value_table_t value_string_table = {.user = (uintptr_t) VALUE_STRING};

  return value_create(&string, sizeof string, &value_string_table);
}

value_t *value_string_release(const string_t string)
{
  static value_table_t value_string_table = {.user = (uintptr_t) VALUE_STRING,
                                                   .destroy = value_string_release_string};

  return value_create(&string, sizeof string, &value_string_table);
}

bool value_stringp(const value_t *string)
{
  return value_type(string) == VALUE_STRING;
}

string_t value_string_get(const value_t *string)
{
  return value_stringp(string) ? *(string_t *) string : string_null();
}

char *value_string_base(const value_t *string)
{
  return string_base(*(string_t *) string);
}

size_t value_string_size(const value_t *string)
{
  return string_size(*(string_t *) string);
}

/* array */

const char *const VALUE_ARRAY = "array";

static void value_array_release_element(void *arg)
{
  value_release(*(value_t **) arg);
}

static void value_array_destroy(value_t *array)
{
  vector_destruct(array, value_array_release_element);
}

value_t *value_array(void)
{
  static value_table_t value_array_table = {.user = (uintptr_t) VALUE_ARRAY, .destroy = value_array_destroy};
  vector_t *vector;

  vector = value_create(NULL, sizeof *vector, &value_array_table);
  vector_construct(vector, sizeof(value_t *));
  return vector;
}

bool value_arrayp(const value_t *array)
{
  return value_type(array) == VALUE_ARRAY;
}

size_t value_array_length(const value_t *array)
{
  return value_arrayp(array) ? vector_size((vector_t *) array) : 0;
}

void value_array_append(value_t *array, value_t *element)
{
  if (!value_arrayp(array))
    return;
  value_hold(element);
  vector_push_back(array, &element);
}

void value_array_append_release(value_t *array, value_t *element)
{
  value_array_append(array, element);
  value_release(element);
}

value_t *value_array_get(const value_t *array, size_t index)
{
  if (!value_arrayp(array))
    return NULL;
  return *(value_t **) vector_at((vector_t *) array, index);
}

void value_array_remove(value_t *array, size_t index)
{
  if (!value_arrayp(array))
    return;
  vector_erase((vector_t *) array, index, value_array_release_element);
}

/* object */

const char *const VALUE_OBJECT = "object";

static void value_object_release_entry(mapd_entry_t *entry)
{
  string_release(entry->key);
  value_release((value_t *) entry->value);
}

static void value_object_release(void *arg)
{
  mapd_destruct(arg, value_object_release_entry);
}

value_t *value_object(void)
{
  static value_table_t value_object_table = {.user = (uintptr_t) VALUE_OBJECT, .destroy = value_object_release};
  mapd_t *mapd;

  mapd = value_create(NULL, sizeof *mapd, &value_object_table);
  mapd_construct(mapd);
  return mapd;
}

bool value_objectp(const value_t *object)
{
  return value_type(object) == VALUE_OBJECT;
}

size_t value_object_size(const value_t *object)
{
  return value_objectp(object) ? mapd_size((mapd_t *) object) : 0;
}

void value_object_set(value_t *object, const string_t key, value_t *value)
{
  if (!value_objectp(object))
    return;
  value_object_delete(object, key);
  value_hold(value);
  mapd_insert(object, string_copy(key), (uintptr_t) value, value_object_release_entry);
}

void value_object_set_release(value_t *object, string_t key, value_t *value)
{
  value_object_set(object, key, value);
  value_release(value);
}

value_t *value_object_get(const value_t *object, const string_t key)
{
  value_t *value;

  if (!value_objectp(object))
    return value_undefined();

  value = (value_t *) mapd_at(object, key);
  return value ? value : value_undefined();
}

void value_object_delete(value_t *object, const string_t key)
{
  if (!value_objectp(object))
    return;

  mapd_erase((mapd_t *) object, key, value_object_release_entry);
}

int value_object_keys_compare(const void *p1, const void *p2)
{
  const data_t d1 = **(data_t **) p1, d2 = **(data_t **) p2;
  size_t s1 = data_size(d1), s2 = data_size(d2);
  int result;

  result = memcmp(data_base(d1), data_base(d2), MIN(s1, s2));
  if (result)
    return result;
  return s1 > s2 ? 1 : -1;
}

value_t *value_object_keys(const value_t *object)
{
  value_t *array, *value;
  string_t key;

  array = value_array();
  value_object_foreach(object, key, value)
  {
    (void) value;
    value_array_append_release(array, value_string_constant(key));
  }
  vector_sort((vector_t *) array, value_object_keys_compare);
  return array;
}
