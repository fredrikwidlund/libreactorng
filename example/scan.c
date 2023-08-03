#include <stdio.h>
#include <reactor.h>

struct state
{
  timeout_t timeout;
  list_t ports;
  int ref;
};

struct port
{
  struct state *state;
  int port;
  bool open;
  network_t connect;
};

const int top[] = {
  21,22,23,25,26,53,80,81,110,111,113,135,139,143,179,199,443,445,465,514,515,
  548,554,587,646,993,995,1025,1026,1027,1433,1720,1723,2000,2001,3306,3389,
  5060,5666,5900,6001,8000,8008,8080,8443,8888,10000,32768,49152,49154
};

static void callback(reactor_event_t *event)
{
  struct port *port = event->state;

  port->connect = 0;
  if (event->type == NETWORK_CONNECT)
  {
    port->open = true;
    reactor_close(NULL, NULL, event->data);
  }
  port->state->ref--;
  if (!port->state->ref)
    timeout_destruct(&port->state->timeout);
}

static void timeout(reactor_event_t *event)
{
  struct state *state = event->state;
  struct port *port;

  list_foreach(&state->ports, port)
    if (port->connect)
      network_cancel(port->connect);
  timeout_destruct(&state->timeout);
}

void scan(char *host)
{
  struct state state = {0};
  struct port *port;
  size_t i;

  timeout_construct(&state.timeout, timeout, &state);
  timeout_set(&state.timeout, reactor_now() + 1000000000, 0);
  list_construct(&state.ports);
  for (i = 0; i < sizeof top / sizeof *top; i++)
  {
    port = list_push_back(&state.ports, NULL, sizeof (struct port));
    port->port = top[i];
    port->state = &state;
    port->connect = network_connect(callback, port, host, top[i]);
    state.ref++;
  }
  reactor_loop();
  printf("[%s]", host);
  list_foreach(&state.ports, port)
    if (port->open)
      printf(" %d", port->port);
  printf("\n");
  list_destruct(&state.ports, NULL);
}

int main(int argc, char **argv)
{
  int i;

  reactor_construct();
  for (i = 1; i < argc; i ++)
    scan(argv[i]);
  reactor_destruct();
}
