#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <err.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/io_uring.h>
#include <reactor.h>

const char request[] =
  "GET / HTTP/1.1\r\n"
  "Host: localhost\r\n"
  "\r\n";

struct state
{
  int seconds;
  size_t requests;
};

struct client
{
  network_t connect;
  int socket;
  char input[1024];
  struct state *state;
};

static void client_request(struct client *);

static void client_recv(reactor_event_t *event)
{
  struct client *client = event->state;
  int result = event->data;

  assert(result > 0);
  client->state->requests++;
  client_request(client);
}

static void client_send(reactor_event_t *event)
{
  struct client *client = event->state;
  int result = event->data;

  assert(result > 0);
  reactor_recv(client_recv, client, client->socket, client->input, sizeof client->input, 0);
}

static void client_request(struct client *client)
{
  reactor_send(client_send, client, client->socket, request, strlen(request), 0);
}

static void client_connect(reactor_event_t *event)
{
  struct client *client = event->state;
  int result = event->data;

  switch (event->type)
  {
  case NETWORK_CONNECT:
    client->socket = result;
    client_request(client);
    break;
  default:
    err(1, "unable to connect");
  }
}

static struct client *client_create(struct state *state, const char *host, int port)
{
  struct client *client;

  client = malloc(sizeof *client);
  client->state = state;
  client->connect = network_connect(client_connect, client, host, port);
  return client;
}

static void state_timeout(reactor_event_t *event)
{
  struct state *state = event->state;

  printf("requests per seconds: %lu\n", state->requests / state->seconds);
  exit(0);
}

int main()
{
  struct state state = {.seconds = 1};
  struct timespec tv = {.tv_sec = state.seconds};
  int i, n = 256;

  reactor_construct();
  printf("running %d clients for %d seconds\n", n, state.seconds);
  for (i = 0; i < n; i ++)
    client_create(&state, "127.0.0.1", 80);

  reactor_timeout(state_timeout, &state, &tv, 0, 0);
  reactor_loop();
  reactor_destruct();
}
