#include <reactor.h>

static void handle_request(reactor_event_t *event)
{
  server_plain((server_session_t *) event->data, string("hello"), NULL, 0);
}

int main()
{
  server_t server;

  reactor_construct();
  server_construct(&server, handle_request, NULL);
  server_open(&server, "127.0.0.1", 80);
  reactor_loop();
  reactor_destruct();
}
