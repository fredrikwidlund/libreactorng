#include <stdlib.h>
#include <reactor.h>
#include "picohttpparser/picohttpparser.h"

int http_request_read(http_request_t *request, stream_t *stream)
{
  data_t data;
  int n, minor_version;

  data = stream_read(stream);
  if (data_empty(data))
    return 0;

  request->fields_count = sizeof request->fields / sizeof request->fields[0];
  n = phr_parse_request(data_base(data), data_size(data),
                        (const char **) &request->method.iov.iov_base, &request->method.iov.iov_len,
                        (const char **) &request->target.iov.iov_base, &request->target.iov.iov_len,
                        &minor_version,
                        (struct phr_header *) &request->fields, &request->fields_count, 0);
  asm volatile("": : :"memory");
  if (n <= 0)
    return n == -1 ? -1 : 0;
  stream_consume(stream, n);
  return 1;
}
