#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <reactor.h>

_Atomic int counter = 0;

static void sender(reactor_event_t *event)
{
  queue_t *queue = event->state;
  queue_producer_t producer;
  int i;

  if (event->type == REACTOR_RETURN)
    return;

  reactor_construct();
  queue_producer_construct(&producer);
  queue_producer_open(&producer, queue);
  for (i = 0; i < 100; i ++)
    queue_producer_push(&producer, (int []){42});
  reactor_loop();
  queue_producer_destruct(&producer);
  reactor_destruct();
}

static void receiver_event(reactor_event_t *event)
{
  int *element = (int *) event->data;

  assert(*element == 42);
  if (__sync_add_and_fetch(&counter, 1) == 10000)
    exit(0);
}

static void receiver(reactor_event_t *event)
{
  queue_t *queue = event->state;
  queue_consumer_t consumer;

  if (event->type == REACTOR_RETURN)
    return;

  reactor_construct();
  queue_consumer_construct(&consumer, receiver_event, &consumer);
  queue_consumer_open(&consumer, queue, 10);
  queue_consumer_pop(&consumer);
  reactor_loop();
  queue_consumer_destruct(&consumer);
  reactor_destruct();
}

int main()
{
  queue_t queue;
  int i;

  reactor_construct();
  queue_construct(&queue, sizeof(int));
  for (i = 0; i < 100; i++)
    reactor_async(sender, &queue);
  for (i = 0; i < 100; i++)
    reactor_async(receiver, &queue);
  reactor_loop();
  queue_destruct(&queue);
  reactor_destruct();
}
