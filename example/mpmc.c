#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <threads.h>
#include <unistd.h>
#include <err.h>
#include <sys/socket.h>

struct q
{
  int fd[2];
  int n;
  _Atomic int producer;
  _Atomic int consumer;
  _Atomic int count;
};

int producer(void *arg)
{
  struct q *q = arg;
  int i, t, id = __sync_add_and_fetch(&q->producer, 1);
  char data[q->n  * 256];
  ssize_t n;

  for (i = 0; i < q->n * 256; i++)
    data[i] = i % q->n;

  t = 1;
  for (i = 0; i < 1000;)
  {
    n = write(q->fd[1], data, q->n * t);
    //n = send(q->fd[1], data, q->n, 0);
    if (n != q->n * t)
    {
      printf("send %d -> %ld\n", q->n, n);
      err(1, "send");
    }
    i += t;
  }

  (void) id;
  //printf("[p%d] done\n", id);
  return 0;
}

int consumer(void *arg)
{
  static int calls = 0;
  struct q *q = arg;
  int i, id = __sync_add_and_fetch(&q->consumer, 1);
  char data[q->n * 1];
  ssize_t n;

  while (1)
  {
    //n = recv(q->fd[0], data, sizeof data, 0);
    n = read(q->fd[0], data, sizeof data);
    calls++;
    if (n % q->n != 0)
    {
      printf("read %ld (align %d)\n", n, q->n);
      exit(1);
    }
    for (i = 0; i < n; i++)
      assert(data[i] == i % q->n);
    i = __sync_add_and_fetch(&q->count, n / q->n);
    if (i == 1000000)
    {
      printf("id %d done, %d calls\n", id, calls);
      exit(0);
    }
    //assert(n == q->n);
  }

  //printf("[c%d] done\n", id);
  return 0;
}

int main()
{
  struct q q = {.n = 8};
  thrd_t t;
  int i;

  assert(pipe(q.fd) == 0);
  //assert(socketpair(AF_UNIX, SOCK_STREAM, 0, q.fd) == 0);
  //assert(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, q.fd) == 0);
  //assert(socketpair(AF_UNIX, SOCK_DGRAM, 0, q.fd) == 0);
  //assert(socketpair(AF_UNIX, SOCK_STREAM, 0, q.fd) == 0);

  for (i = 0; i < 1000; i++)
  {
    assert(thrd_create(&t, producer, &q) == 0);
    assert(thrd_detach(t) == 0);
  }

  sleep(1);
  for (i = 0; i < 10; i++)
  {
    assert(thrd_create(&t, consumer, &q) == 0);
    assert(thrd_detach(t) == 0);
  }

  sleep(1000);
}
