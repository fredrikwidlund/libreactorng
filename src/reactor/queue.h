#ifndef REACTOR_QUEUE_H_INCLUDED
#define REACTOR_QUEUE_H_INCLUDED

#include <stdlib.h>

enum
{
  QUEUE_CONSUMER_POP
};

typedef struct queue          queue_t;
typedef struct queue_consumer queue_consumer_t;
typedef struct queue_producer queue_producer_t;

struct queue
{
  int             fd[2];
  size_t          element_size;
};

struct queue_producer
{
  queue_t        *queue;
  buffer_t        output;
  size_t          output_offset;
  buffer_t        output_queued;
  reactor_t       write;
};

struct queue_consumer
{
  reactor_user_t  user;
  queue_t        *queue;
  buffer_t        input;
  reactor_t       read;
  bool           *abort;
};

void  queue_construct(queue_t *, size_t);
void  queue_destruct(queue_t *);

void  queue_producer_construct(queue_producer_t *);
void  queue_producer_destruct(queue_producer_t *);
void  queue_producer_open(queue_producer_t *, queue_t *);
void  queue_producer_push(queue_producer_t *, void *);
void  queue_producer_push_sync(queue_producer_t *, void *);
void  queue_producer_close(queue_producer_t *);

void  queue_consumer_construct(queue_consumer_t *, reactor_callback_t *, void *);
void  queue_consumer_destruct(queue_consumer_t *);
void  queue_consumer_open(queue_consumer_t *, queue_t *, size_t);
void  queue_consumer_pop(queue_consumer_t *);
void *queue_consumer_pop_sync(queue_consumer_t *);
void  queue_consumer_close(queue_consumer_t *);

#endif /* REACTOR_QUEUE_H_INCLUDED */
