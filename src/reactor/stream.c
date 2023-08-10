#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <assert.h>
#include <reactor.h>

const size_t STREAM_BLOCK_SIZE = 16384;

enum
{
  STREAM_TYPE_SOCKET,
  STREAM_TYPE_FILE
};

static void stream_input(stream_t *);
static void stream_output(stream_t *);

static void stream_release_buffer(reactor_event_t *event)
{
  free(event->state);
}

static void stream_recv(reactor_event_t *event)
{
  stream_t *stream = event->state;
  int result = event->data;

  stream->read = 0;
  if (result > 0)
  {
    stream->input_offset += result;
    buffer_resize(&stream->input, buffer_size(&stream->input) + result);
    stream_input(stream);
    reactor_call(&stream->user, STREAM_READ, 0);
  }
  else
    reactor_call(&stream->user, result == 0 ? STREAM_CLOSE : STREAM_ERROR, -result);
}

static void stream_input(stream_t *stream)
{
  const size_t size = STREAM_BLOCK_SIZE;

  assert(stream->read == 0);
  if (stream->input_consumed)
  {
    buffer_erase(&stream->input, 0, stream->input_consumed);
    stream->input_consumed = 0;
  }

  buffer_reserve(&stream->input, MAX(buffer_size(&stream->input) + size, 2 * size));
  stream->read = stream->type == STREAM_TYPE_SOCKET ?
    reactor_recv(stream_recv, stream, stream->fd, buffer_end(&stream->input), size, 0) :
    reactor_read(stream_recv, stream, stream->fd, buffer_end(&stream->input), size, stream->input_offset);
}

static void stream_send(reactor_event_t *event)
{
  stream_t *stream = event->state;
  int result = event->data;
  buffer_t *buffer;

  stream->write = 0;
  if (result > 0)
  {
    stream->output_offset += result;
    buffer = &stream->output[!stream->output_current];
    assert((size_t) result == buffer_size(buffer));
    buffer_clear(buffer);
    stream_output(stream);
  }
  else
    reactor_call(&stream->user, STREAM_ERROR, -result);
}

static void stream_output(stream_t *stream)
{
  data_t data;
  buffer_t *current, *next;

  if (stream->write || !stream->output_flushed)
    return;

  current = &stream->output[stream->output_current];
  stream->output_current = !stream->output_current;
  next = &stream->output[stream->output_current];
  data = buffer_data(current);

  buffer_append(next, data_offset(data, stream->output_flushed));
  buffer_resize(current, stream->output_flushed);
  stream->output_flushed = 0;

  stream->write = stream->type == STREAM_TYPE_SOCKET ?
    reactor_send(stream_send, stream, stream->fd, data_base(data), data_size(data), 0) :
    reactor_write(stream_send, stream, stream->fd, data_base(data), data_size(data), stream->output_offset);

}

void stream_construct(stream_t *stream, reactor_callback_t *callback, void *state)
{
  *stream = (stream_t) {.user = reactor_user_define(callback, state), .fd = -1};
  buffer_construct(&stream->input);
  buffer_construct(&stream->output[0]);
  buffer_construct(&stream->output[1]);
}

void stream_destruct(stream_t *stream)
{
  stream_close(stream);
  buffer_destruct(&stream->input);
  buffer_destruct(&stream->output[0]);
  buffer_destruct(&stream->output[1]);
}

void stream_open(stream_t *stream, int fd, int flags)
{
  struct stat st;
  int e;

  stream->fd = fd;
  e = fstat(fd, &st);
  assert(e == 0);
  stream->type = S_ISSOCK(st.st_mode) ? STREAM_TYPE_SOCKET : STREAM_TYPE_FILE;
  if (!(flags & STREAM_WRITE_ONLY))
    stream_input(stream);
}

void stream_close(stream_t *stream)
{
  if (stream->read)
  {
    reactor_cancel(stream->read, stream_release_buffer, buffer_deconstruct(&stream->input));
    stream->read = 0;
  }
  if (stream->write)
  {
    reactor_cancel(stream->write, stream_release_buffer, buffer_deconstruct(&stream->output[!stream->output_current]));
    stream->write = 0;
  }
  if (stream->fd >= 0)
  {
    reactor_close(NULL, NULL, stream->fd);
    stream->fd = -1;
  }
}

data_t stream_read(stream_t *stream)
{
  return data_offset(buffer_data(&stream->input), stream->input_consumed);
}

void stream_consume(stream_t *stream, size_t size)
{
  stream->input_consumed += size;
}

void *stream_allocate(stream_t *stream, size_t size)
{
  return data_base(buffer_allocate(&stream->output[stream->output_current], size));
}

void stream_write(stream_t *stream, data_t data)
{
  memcpy(stream_allocate(stream, data_size(data)), data_base(data), data_size(data));
}

void stream_flush(stream_t *stream)
{
  stream->output_flushed = buffer_size(&stream->output[stream->output_current]);
  stream_output(stream);
}
