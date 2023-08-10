#include <stdio.h>
#include <string.h>
#include <err.h>
#include <reactor.h>

static void input(reactor_event_t *event)
{
  stream_t *stream = event->state;
  data_t data;

  switch (event->type)
  {
  case STREAM_ERROR:
    errx(1, "error: %s\n", strerror(event->data));
    break;
  case STREAM_READ:
    data = stream_read(stream);
    stream_consume(stream, data_size(data));
    fwrite(data_base(data), data_size(data), 1, stdout);
    break;
  case STREAM_CLOSE:
    stream_destruct(stream);
    break;
  }
}

int main()
{
  stream_t s;

  reactor_construct();
  stream_construct(&s, input, &s);
  stream_open(&s, 0);
  reactor_loop();
  reactor_destruct();
}
