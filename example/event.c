#include <stdio.h>
#include <reactor.h>

void callback(reactor_event_t *reactor_event)
{
  event_t *event = reactor_event->state;

  printf("signal received\n");
  event_close(event);
}

void producer(reactor_event_t *reactor_event)
{
  event_t *event = reactor_event->state;

  if (reactor_event->type == REACTOR_CALL)
    event_signal_sync(event);
}

int main()
{
  event_t event;

  reactor_construct();
  event_construct(&event, callback, &event);
  event_open(&event);
  reactor_async(producer, &event);
  reactor_loop();
  event_destruct(&event);
  reactor_destruct();
}
