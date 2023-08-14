#include <stdio.h>
#include <err.h>
#include <reactor.h>

static void handle_request(reactor_event_t *event)
{
  server_session_t *session = (server_session_t *) event->data;

  if (reactor_unlikely(event->type != SERVER_REQUEST))
    errx(1, "server error");
  server_hello(session);
}

int main()
{
  server_t server;

  reactor_construct();
  server_construct(&server, handle_request, &server);
  server_open(&server, "127.0.0.1", 80);
  reactor_loop();
  server_destruct(&server);
  reactor_destruct();
}
