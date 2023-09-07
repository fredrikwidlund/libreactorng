#include <err.h>
#include <assert.h>
#include <reactor.h>

static void callback(reactor_event_t *event)
{
  value_t *message = (value_t *) event->data;

  fprintf(stderr, "[log] %s\n", value_string_base(message));
}

int main()
{
  flow_t flow;

  reactor_construct();
  flow_construct(&flow, callback, NULL);
  flow_search(&flow, ".");

  assert(flow_node(&flow, "module1", NULL, "<main>", value_null()) == 0);
  assert(flow_node(&flow, "module2", NULL, "<main>", value_null()) == 0);
  flow_connect(&flow, "module1", "module2", value_null());
  flow_start(&flow);

  reactor_loop();

  flow_stop(&flow);
  flow_destruct(&flow);
  reactor_destruct();
}
