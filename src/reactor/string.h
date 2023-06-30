#ifndef REACTOR_STRING_H_INCLUDED
#define REACTOR_STRING_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>

typedef data_t string_t;

/* constructor/destructor */

string_t  string(const char *);
string_t  string_data(const data_t);
string_t  string_null(void);
string_t  string_copy(const string_t);
void      string_release(string_t);

/* capacity */

size_t    string_size(const string_t);
bool      string_empty(string_t);

/* element access */

char     *string_base(const string_t);

/* operations */

bool      string_equal(const string_t, const string_t);
bool      string_equal_case(const string_t, const string_t);

/* utf8 */

size_t    string_utf8_length(const string_t);
uint32_t  string_utf8_get(const char *, const char *, const char **);
uint32_t  string_utf8_get_encoded(const char *, const char **);
void      string_utf8_put(buffer_t *, uint32_t);
void      string_utf8_put_encoded_basic(buffer_t *, uint16_t);
void      string_utf8_put_encoded(buffer_t *, uint32_t);
bool      string_utf8_encode(buffer_t *, const string_t, bool);
string_t  string_utf8_decode(const char *, const char **);

#endif /* REACTOR_STRING_H_INCLUDED */
