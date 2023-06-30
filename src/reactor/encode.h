#ifndef VALUE_ENCODE_H_INCLUDED
#define VALUE_ENCODE_H_INCLUDED

enum value_encode_options
{
  VALUE_ENCODE_PRETTY = 1,
  VALUE_ENCODE_ASCII  = 2,
};

typedef enum value_encode_options value_encode_options_t;

bool     value_buffer_encode(buffer_t *, const value_t *, value_encode_options_t);
string_t value_encode(const value_t *, value_encode_options_t);

#endif /* VALUE_ENCODE_H_INCLUDED */
