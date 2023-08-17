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

static void stream_update(stream_t *);

static void stream_release_buffer(reactor_event_t *event)
{
  free(event->state);
}

static void stream_recv(reactor_event_t *event)
{
  stream_t *stream = event->state;
  int result = event->data;
  bool abort = false;

  stream->read = 0;
  if (reactor_unlikely(result <= 0))
  {
    reactor_call(&stream->user, result == 0 ? STREAM_CLOSE : STREAM_ERROR, -result);
    return;
  }

  stream->input_offset += result;
  buffer_resize(&stream->input, buffer_size(&stream->input) + result);
  stream->abort = &abort;
  reactor_call(&stream->user, STREAM_READ, 0);
  if (reactor_unlikely(abort))
    return;
  stream->abort = NULL;
  stream_update(stream);
}

static void stream_send(reactor_event_t *event)
{
  stream_t *stream = event->state;
  int result = event->data;

  stream->write = 0;
  if (result <= 0)
  {
    reactor_call(&stream->user, STREAM_ERROR, -result);
    return;
  }

  stream->output_offset += result;
  stream->output_partial += result;
  if (stream->output_partial == buffer_size(&stream->output_writing))
    stream->output_partial = 0;
  stream_update(stream);
}

static void stream_input(stream_t *stream)
{
  const size_t size = STREAM_BLOCK_SIZE;

  if (reactor_likely(stream->input_consumed))
  {
    buffer_erase(&stream->input, 0, stream->input_consumed);
    stream->input_consumed = 0;
  }

  buffer_reserve(&stream->input, buffer_size(&stream->input) + size);

#ifdef UNIT_TESTING
  memset(buffer_end(&stream->input), 0, size);
#endif

  stream->read = reactor_likely(stream->type == STREAM_TYPE_SOCKET) ?
    reactor_recv(stream_recv, stream, stream->fd, buffer_end(&stream->input), size, 0) :
    reactor_read(stream_recv, stream, stream->fd, buffer_end(&stream->input), size, stream->input_offset);
}


static void stream_output(stream_t *stream)
{
  data_t data;

  data = data_offset(buffer_data(&stream->output_writing), stream->output_partial);
  stream->write = reactor_likely(stream->type == STREAM_TYPE_SOCKET) ?
    reactor_send(stream_send, stream, stream->fd, data_base(data), data_size(data), 0) :
    reactor_write(stream_send, stream, stream->fd, data_base(data), data_size(data), stream->output_offset);
}

static void stream_update(stream_t *stream)
{
  if (reactor_unlikely(!stream_is_open(stream)) || stream->write)
    return;

  if (stream->output_partial)
  {
    stream_output(stream);
  }
  else if (stream->output_flushed)
  {
    stream->output_partial = 0;
    buffer_clear(&stream->output_writing);
    buffer_append(&stream->output_writing, data_offset(buffer_data(&stream->output_waiting), stream->output_flushed));
    buffer_resize(&stream->output_waiting, stream->output_flushed);
    buffer_switch(&stream->output_waiting, &stream->output_writing);
    stream->output_flushed = 0;
    stream_output(stream);
  }
  else if (!stream->read && (!stream->flags & STREAM_WRITE_ONLY))
  {
    stream_input(stream);
  }
}

void stream_construct(stream_t *stream, reactor_callback_t *callback, void *state)
{
  *stream = (stream_t) {.user = reactor_user_define(callback, state), .fd = -1};
  buffer_construct(&stream->input);
  buffer_construct(&stream->output_writing);
  buffer_construct(&stream->output_waiting);
}

void stream_destruct(stream_t *stream)
{
  stream_close(stream);
  if (stream->abort)
    *stream->abort = true;
  buffer_destruct(&stream->input);
  buffer_destruct(&stream->output_writing);
  buffer_destruct(&stream->output_waiting);
}

void stream_open(stream_t *stream, int fd, int flags)
{
  struct stat st;
  int e;

  stream->fd = fd;
  stream->flags = flags;
  e = fstat(fd, &st);
  assert(e == 0);
  stream->type = S_ISSOCK(st.st_mode) ? STREAM_TYPE_SOCKET : STREAM_TYPE_FILE;
  stream_update(stream);
}

int stream_fd(stream_t *stream)
{
  return stream->fd;
}

bool stream_is_open(stream_t *stream)
{
  return stream->fd >= 0;
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
    reactor_cancel(stream->write, stream_release_buffer, buffer_deconstruct(&stream->output_writing));
    stream->write = 0;
  }
  if (stream->fd >= 0)
  {
    reactor_close(NULL, NULL, stream->fd);
    stream->fd = -1;
  }
}

inline __attribute__((always_inline, flatten))
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
  return data_base(buffer_allocate(&stream->output_waiting, size));
}

void stream_write(stream_t *stream, data_t data)
{
  memcpy(stream_allocate(stream, data_size(data)), data_base(data), data_size(data));
}

void stream_flush(stream_t *stream)
{
  stream->output_flushed = buffer_size(&stream->output_waiting);
  stream_update(stream);
}
