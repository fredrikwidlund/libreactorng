#ifndef REACTOR_STREAM_H_INCLUDED
#define REACTOR_STREAM_H_INCLUDED

#include <stdlib.h>
#include <reactor.h>

enum
{
  STREAM_ERROR,
  STREAM_READ,
  STREAM_CLOSE
};

enum
{
  STREAM_WRITE_ONLY = 0x01
};

typedef struct stream stream_t;

struct stream
{
  reactor_user_t  user;
  int             fd;
  int             type;
  int             flags;
  bool           *abort;

  reactor_t       read;
  size_t          input_offset;
  buffer_t        input;
  size_t          input_consumed;

  reactor_t       write;
  size_t          output_offset;
  size_t          output_partial;
  buffer_t        output_writing;
  buffer_t        output_waiting;
  size_t          output_flushed;
};

void    stream_construct(stream_t *, reactor_callback_t *, void *);
void    stream_destruct(stream_t *);
void    stream_open(stream_t *, int, int);
int     stream_fd(stream_t *);
bool    stream_is_open(stream_t *);
void    stream_close(stream_t *);
data_t  stream_read(stream_t *);
void    stream_consume(stream_t *, size_t);
void   *stream_allocate(stream_t *, size_t);
void    stream_write(stream_t *, data_t);
void    stream_flush(stream_t *);

#endif /* REACTOR_STREAM_H_INCLUDED */
