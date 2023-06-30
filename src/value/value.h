#ifndef VALUE_VALUE_H_INCLUDED
#define VALUE_VALUE_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>

typedef void               value_t;
typedef void               value_destroy_t(value_t *);
typedef struct value_table value_table_t;

struct value_table
{
  uintptr_t        user;
  value_destroy_t *destroy;
};

value_t     *value_create(const void *, size_t, const value_table_t *);
uintptr_t    value_user(const value_t *);
size_t       value_ref(const value_t *);
void         value_hold(value_t *);
void         value_release(value_t *);
uintptr_t    value_data(const value_t *);
const char  *value_type(const value_t *);

/* undefined */

extern const char VALUE_UNDEFINED[];

value_t     *value_undefined(void);
bool         value_is_undefined(const value_t *);

/* null */

extern const char VALUE_NULL[];

value_t     *value_null(void);
bool         value_is_null(const value_t *);

/* bool */

extern const char VALUE_BOOL[];

value_t     *value_bool(bool);
bool         value_is_bool(const value_t *);
bool         value_bool_get(const value_t *);

/* number */

extern const char VALUE_NUMBER[];

value_t     *value_number(long double);
bool         value_is_number(const value_t *);
long double  value_number_get(const value_t *);

/* string */

extern const char VALUE_STRING[];

value_t     *value_string(const string_t);
value_t     *value_string_constant(const string_t);
value_t     *value_string_release(const string_t);
bool         value_is_string(const value_t *);
string_t     value_string_get(const value_t *);
char        *value_string_base(const value_t *);
size_t       value_string_size(const value_t *);

/* array */

extern const char VALUE_ARRAY[];

#define value_array_foreach(array, element)                             \
  if (value_is_array(array))                                            \
    for (size_t __value_iter = 0; __value_iter < vector_size(array) &&  \
           (element = *(void **) vector_at(array, __value_iter));       \
         __value_iter++)

value_t     *value_array(void);
bool         value_is_array(const value_t *);
size_t       value_array_length(const value_t *);
void         value_array_append(value_t *, value_t *);
void         value_array_append_release(value_t *, value_t *);
value_t     *value_array_get(const value_t *, size_t);
void         value_array_remove(value_t *, size_t);

/* object */

extern const char VALUE_OBJECT[];

#define value_object_foreach(object, key, value)                                                             \
  if (value_is_object(object))                                                                               \
    for (size_t __value_iter = 0; __value_iter < ((mapd_t *) object)->map.elements_capacity; __value_iter++) \
      if ((value) = (value_t *) ((mapd_entry_t *)((mapd_t *) (object))->map.elements)[__value_iter].value,   \
          data_base((key) = ((mapd_entry_t *)((mapd_t *) (object))->map.elements)[__value_iter].key) != NULL)

value_t     *value_object(void);
bool         value_is_object(const value_t *);
size_t       value_object_size(const value_t *);
void         value_object_set(value_t *, const string_t, value_t *);
void         value_object_set_release(value_t *, string_t, value_t *);
value_t     *value_object_get(const value_t *, const string_t);
void         value_object_delete(value_t *, const string_t);
value_t     *value_object_keys(const value_t *);

#endif /* VALUE_VALUE_H_INCLUDED */
