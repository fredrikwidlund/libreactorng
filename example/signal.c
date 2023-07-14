#include <stdio.h>
#include <reactor.h>

void callback(reactor_event_t *event)
{
  signal_t *signal = event->state;

  printf("signal received\n");
  signal_close(signal);
}

void producer(reactor_event_t *event)
{
  signal_t *signal = event->state;

  if (event->type == REACTOR_CALL)
    signal_send_sync(signal);
}

int main()
{
  signal_t signal;

  reactor_construct();
  signal_construct(&signal, callback, &signal);
  signal_open(&signal);
  reactor_async(producer, &signal);
  reactor_loop();
  signal_destruct(&signal);
  reactor_destruct();
}
