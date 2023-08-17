#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

typedef struct http_request http_request_t;
typedef struct http_response http_response_t;

typedef struct http_field http_field_t;

struct http_field
{
  string_t     name;
  string_t     value;
};

struct http_request
{
  string_t     method;
  string_t     target;
  data_t       body;
  http_field_t fields[16];
  size_t       fields_count;
};

struct http_response
{
  string_t     status;
  string_t     date;
  string_t     type;
  data_t       body;
  http_field_t fields[16];
  size_t       fields_count;
};

http_field_t http_field_define(string_t, string_t);
string_t     http_field_lookup(http_field_t *, size_t, string_t);
int          http_read_request(stream_t *, string_t *, string_t *, data_t *, http_field_t *, size_t *);
void         http_write_response(stream_t *, string_t, string_t, string_t, data_t, http_field_t *, size_t);

#endif /* REACTOR_HTTP_H_INCLUDED */
