#include <stdio.h>
#include <reactor.h>

static void receive(void *state, value_t *message)
{
  (void) state;

  printf("[module2 instance pulse: %d\n", (int) value_number_get(message));
}

flow_table_t module = {.receive = receive};
