#include <reactor.h>

void callback(reactor_event_t *event)
{
  printf("read returned %d\n", (int) event->data);
}

int main()
{
  char buffer[1024];

  reactor_construct();
  reactor_read(callback, NULL, 0, buffer, sizeof buffer, 0);
  reactor_loop();
  reactor_destruct();
}
