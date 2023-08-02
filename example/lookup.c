#include <stdio.h>
#include <assert.h>
#include <reactor.h>

int main()
{
  reactor_construct();
  network_resolve(NULL, NULL, "www.sunet.se");
  reactor_loop();
  reactor_destruct();
}
