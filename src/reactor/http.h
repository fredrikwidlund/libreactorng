#ifndef REACTOR_HTTP_H_INCLUDED
#define REACTOR_HTTP_H_INCLUDED

typedef struct http_request http_request_t;
typedef struct http_response http_response_t;

typedef struct http_field http_field_t;

struct http_field
{
  data_t       name;
  data_t       value;
};

struct http_request
{
  data_t       method;
  data_t       target;
  data_t       body;
  http_field_t fields[16];
  size_t       fields_count;
};

struct http_response
{
  data_t       status;
  data_t       date;
  data_t       type;
  data_t       body;
  http_field_t fields[16];
  size_t       fields_count;
};

int http_request_read(http_request_t *, stream_t *);

#endif /* REACTOR_HTTP_H_INCLUDED */
