#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "../reactor.h"

/* queue */

static void queue_cancel(reactor_event_t *event)
{
  free(event->state);
}

void queue_construct(queue_t *queue, size_t element_size)
{
  *queue = (struct queue) {.element_size = element_size};
  assert(pipe(queue->fd) == 0);
}

void queue_destruct(queue_t *queue)
{
  (void) close(queue->fd[0]);
  (void) close(queue->fd[1]);
}

/* queue producer */

static void queue_producer_update(queue_producer_t *);

#ifndef UNIT_TESTING
static
#endif
void queue_producer_write(reactor_event_t *event)
{
  queue_producer_t *producer = event->state;
  int result = event->data;

  producer->write = 0;
  if (result == -EINTR)
  {
    queue_producer_update(producer);
    return;
  }
  assert(result > 0 && result % producer->queue->element_size == 0);
  producer->output_offset += result;
  queue_producer_update(producer);
}

static void queue_producer_update(queue_producer_t *producer)
{
  buffer_t buffer;
  size_t size, size_max;

  if (producer->write)
    return;

  if (producer->output_offset == buffer_size(&producer->output))
  {
    buffer_clear(&producer->output);
    producer->output_offset = 0;
    buffer = producer->output;
    producer->output = producer->output_queued;
    producer->output_queued = buffer;
  }

  if (!buffer_size(&producer->output))
    return;

  assert(producer->queue->fd[1] >= 0);
  size_max = producer->queue->element_size * 128;
  size = buffer_size(&producer->output) - producer->output_offset;
  if (size > size_max)
    size = size_max;
  producer->write = reactor_write(queue_producer_write, producer, producer->queue->fd[1],
                                  (char *) buffer_base(&producer->output) + producer->output_offset, size, 0);
}

void queue_producer_construct(queue_producer_t *producer)
{
  *producer = (queue_producer_t) {0};

  buffer_construct(&producer->output);
  buffer_construct(&producer->output_queued);
}

void queue_producer_destruct(queue_producer_t *producer)
{
  queue_producer_close(producer);
}

void queue_producer_open(queue_producer_t *producer, queue_t *queue)
{
  producer->queue = queue;
}

void queue_producer_push(queue_producer_t *producer, void *element)
{
  buffer_insert(&producer->output_queued, buffer_size(&producer->output_queued), data(element, producer->queue->element_size));
  queue_producer_update(producer);
}

void queue_producer_push_sync(queue_producer_t *producer, void *element)
{
  assert(write(producer->queue->fd[1], element, producer->queue->element_size) == (ssize_t) producer->queue->element_size);
}

void queue_producer_close(queue_producer_t *producer)
{
  if (!producer->queue)
    return;
  producer->queue = NULL;
  if (producer->write)
  {
    reactor_cancel(producer->write, queue_cancel, buffer_deconstruct(&producer->output));
    producer->write = 0;
  }
  buffer_destruct(&producer->output);
  buffer_destruct(&producer->output_queued);
}

/* queue consumer */

static void queue_consumer_update(queue_consumer_t *);

#ifndef UNIT_TESTING
static
#endif
void queue_consumer_read(reactor_event_t *event)
{
  queue_consumer_t *consumer = event->state;
  int offset, result = event->data;
  bool abort = false;

  consumer->read = 0;
  if (result == -EINTR)
  {
    queue_consumer_update(consumer);
    return;
  }
  assert(result > 0 && result % consumer->queue->element_size == 0);
  for (offset = 0; offset < result; offset += consumer->queue->element_size)
  {
    consumer->abort = &abort;
    reactor_call(&consumer->user, QUEUE_CONSUMER_POP, (uint64_t) buffer_base(&consumer->input) + offset);
    if (abort)
      return;
    consumer->abort = NULL;
  }
  queue_consumer_update(consumer);
}

static void queue_consumer_update(queue_consumer_t *consumer)
{
  if (consumer->read)
    return;
  consumer->read = reactor_read(queue_consumer_read, consumer, consumer->queue->fd[0],
                                buffer_base(&consumer->input), buffer_size(&consumer->input), 0);
}

void queue_consumer_construct(queue_consumer_t *consumer, reactor_callback_t *callback, void *state)
{
  *consumer = (queue_consumer_t) {.user = reactor_user_define(callback, state)};
  buffer_construct(&consumer->input);
}

void queue_consumer_destruct(queue_consumer_t *consumer)
{
  queue_consumer_close(consumer);
  buffer_destruct(&consumer->input);
}

void queue_consumer_open(queue_consumer_t *consumer, queue_t *queue, size_t batch_size)
{
  consumer->queue = queue;
  buffer_resize(&consumer->input, queue->element_size * batch_size);
}

void queue_consumer_pop(queue_consumer_t *consumer)
{
  queue_consumer_update(consumer);
}

void *queue_consumer_pop_sync(queue_consumer_t *consumer)
{
  void *element = buffer_base(&consumer->input);

  assert(read(consumer->queue->fd[0], element, consumer->queue->element_size) ==
         (ssize_t) consumer->queue->element_size);
  return element;
}

void queue_consumer_close(queue_consumer_t *consumer)
{
  if (!consumer->queue)
    return;
  if (consumer->abort)
    *consumer->abort = true;
  consumer->queue = NULL;
  if (consumer->read)
  {
    reactor_cancel(consumer->read, queue_cancel, buffer_deconstruct(&consumer->input));
    consumer->read = 0;
  }
}
