#ifndef VALUE_STRING_H_INCLUDED
#define VALUE_STRING_H_INCLUDED

typedef struct string string_t;

struct string
{
  data_t data;
};

string_t string_define(const char *, size_t);
string_t string_null(void);
string_t string_copy(string);
string_t string_read(const char *, const char **);

#endif /* VALUE_STRING_H_INCLUDED */
