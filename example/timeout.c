#include <stdio.h>
#include <reactor.h>

void callback(__attribute__((unused)) reactor_event_t *event)
{
  printf("timeout\n");
}

int main()
{
  timeout_t t;

  reactor_construct();
  timeout_construct(&t, callback, &t);
  timeout_set(&t, reactor_now() + 1000000000, 0);
  reactor_loop();
  reactor_destruct();
}
