#include <reactor.h>

struct state
{
  flow_node_t *node;
  timeout_t    timeout;
  size_t       counter;
};

static void *load(void)
{
  printf("[module1 loading]\n");
  return NULL;
}

static void timeout(reactor_event_t *event)
{
  struct state *state = event->state;

  value_t *message;

  message = value_string(string("module1 sending pulse"));
  flow_log(state->node, message);
  value_release(message);

  flow_send_and_release(state->node, value_number(state->counter));
  state->counter--;
  if (!state->counter)
  {
    timeout_clear(&state->timeout);
    flow_exit(state->node);
  }
}

static void *create(flow_node_t *node, value_t *value)
{
  struct state *state = malloc(sizeof *state);

  (void) value;
  printf("[module1 instance created]\n");
  state->node = node;
  state->counter = 3;
  timeout_construct(&state->timeout, timeout, state);
  timeout_set(&state->timeout, reactor_now(), 1000);
  return state;
}

static void destroy(void *user)
{
  struct state *state = user;

  printf("[module1 instance destroyed]\n");
  timeout_destruct(&state->timeout);
  free(state);
}

flow_table_t module = {.load = load, .create = create, .destroy = destroy};
