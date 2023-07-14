#include <stdio.h>
#include <assert.h>
#include <reactor.h>

extern char *__progname;

static void callback(reactor_event_t *event)
{
  struct addrinfo *ai = (struct addrinfo *) event->data;
  char host[NI_MAXHOST], serv[NI_MAXSERV];
  int e;

  while (ai)
    {
      e = getnameinfo(ai->ai_addr, ai->ai_addrlen, host, sizeof host, serv, sizeof serv, NI_NUMERICHOST);
      assert(e == 0);
      (void) printf("%s\n", host);
      ai = ai->ai_next;
    }
}

int main(int argc, char **argv)
{
  resolver_t r;

  if (argc != 2)
  {
    (void) fprintf(stderr, "usage: %s HOST\n", __progname);
    exit(1);
  }
  reactor_construct();
  resolver_construct(&r, callback, NULL);
  resolver_lookup(&r, 0, AF_INET, SOCK_STREAM, 0, argv[1], "80");
  reactor_loop();
  reactor_destruct();
}
