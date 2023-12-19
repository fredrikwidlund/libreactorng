#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ltdl.h>
#include <reactor.h>

/* flow node */

static void flow_node_receive(flow_node_t *node, value_t *message)
{
  if (node->table->receive)
    node->table->receive(node, message);
}

static void flow_node_consumer(reactor_event_t *event)
{
  flow_node_t *node = event->state;
  value_t *message = *(value_t **) event->data;

  if (message)
  {
    flow_node_receive(node, message);
    value_release(message);
  }
  else /* NULL message means producer is signing off */
  {
     node->consumer_sources--;
    if (!node->consumer_sources)
      queue_consumer_close(&node->consumer);
  }
}

static void flow_node_construct(flow_node_t *node, const char *name, flow_module_t *module, flow_group_t *group,
                                value_t *conf)
{
  *node = (flow_node_t) {.name = strdup(name), .table = module->table, .group = group, .conf = conf};
  value_hold(node->conf);
  list_construct(&node->edges);
  queue_construct(&node->queue, sizeof (value_t *));
  queue_consumer_construct(&node->consumer, flow_node_consumer, node);
}

static void flow_node_connect(flow_node_t *node, flow_node_t *target, value_t *conf)
{
  flow_edge_t *edge;

  edge = list_push_back(&node->edges, NULL, sizeof *edge);
  edge->direct = node->group == target->group;
  edge->target = target;
  edge->conf = conf;
  value_hold(edge->conf);
  queue_producer_construct(&edge->producer);
  if (!edge->direct)
    target->consumer_sources++;
}

static void flow_node_start(flow_node_t *node)
{
  flow_edge_t *edge;

  if (node->consumer_sources)
  {
    queue_consumer_open(&node->consumer, &node->queue, 256);
    queue_consumer_pop(&node->consumer);
  }
  list_foreach(&node->edges, edge)
    if (!edge->direct)
      queue_producer_open(&edge->producer, &edge->target->queue);

  if (node->table->create)
    node->state = node->table->create(node, node->conf);
  node->started = true;
}

static void flow_node_stop(flow_node_t *node)
{
  if (!node->started)
    return;
  node->started = false;
  if (node->table->destroy)
    node->table->destroy(node->state);
  node->state = NULL;
}

static void flow_node_send(flow_node_t *node, value_t *message)
{
  flow_edge_t *edge;

  list_foreach(&node->edges, edge)
    if (edge->direct)
      flow_node_receive(edge->target, message);
  list_foreach(&node->edges, edge)
    if (!edge->direct)
    {
      value_hold(message);
      queue_producer_push(&edge->producer, &message);
    }
}

static void flow_node_release_edge(void *arg)
{
  flow_edge_t *edge = arg;

  value_release(edge->conf);
  queue_producer_destruct(&edge->producer);
}

static void flow_node_destruct(flow_node_t *node)
{
  flow_node_stop(node);
  list_destruct(&node->edges, flow_node_release_edge);
  queue_consumer_destruct(&node->consumer);
  queue_destruct(&node->queue);
  value_release(node->conf);
  free((void *) node->name);
}

/* flow group */

static void flow_group_main(flow_group_t *group)
{
  flow_node_t *node;

  queue_producer_open(&group->log_producer, &group->flow->log_queue);
  reactor_construct();
  list_foreach(&group->nodes, node)
    flow_node_start(node);
  reactor_loop();
  list_foreach(&group->nodes, node)
    flow_node_stop(node);
  reactor_destruct();
  queue_producer_push_sync(&group->log_producer, (value_t *[]){NULL});
}

static void flow_group_async(reactor_event_t *event)
{
  flow_group_t *group = event->state;

  switch (event->type)
  {
  case REACTOR_CALL:
    flow_group_main(group);
    break;
  default:
  case REACTOR_RETURN:
    group->async = 0;
    break;
  }
}

static void flow_group_start(flow_group_t *group)
{
  flow_node_t *node;

  if (group->main_process)
  {
    list_foreach(&group->nodes, node)
      flow_node_start(node);
  }
  else
    group->async = reactor_async(flow_group_async, group);
}

/* flow */

static void flow_release_node(void *arg)
{
  flow_node_destruct(arg);
}

static void flow_release_group(void *arg)
{
  flow_group_t *group = arg;

  list_destruct(&group->nodes, flow_release_node);
  free((void *) group->name);
  if (group->async)
    reactor_cancel(group->async, NULL, NULL);
  queue_producer_destruct(&group->log_producer);
}

static void flow_release_module(void *arg)
{
  flow_module_t *module = arg;

  free((void *) module->name);
  lt_dlclose((void *) module->handle);
}

static void flow_open_module(const char *name, void **handle, flow_table_t **table)
{
  lt_dladvise advise;

  lt_dladvise_init(&advise);
  lt_dladvise_ext(&advise);
  lt_dladvise_global(&advise);
  *handle = lt_dlopenadvise(name, advise);
  lt_dladvise_destroy(&advise);
  *table = lt_dlsym(*handle, "module");
  if (!*table)
  {
    lt_dlclose(*handle);
    *handle = NULL;
    return;
  }
}

static flow_module_t *flow_lookup_module(flow_t *flow, const char *name)
{
  flow_module_t *module;
  void *handle;
  flow_table_t *table;

  list_foreach(&flow->modules, module)
    if (strcmp(module->name, name) == 0)
      return module;

  flow_open_module(name, &handle, &table);
  if (!handle)
    return NULL;

  module = list_push_back(&flow->modules, NULL, sizeof *module);
  module->name = strdup(name);
  module->handle = handle;
  module->table = table;
  return module;
}

static flow_module_t *flow_match_module(flow_t *flow, const char *name)
{
  flow_module_t *module;
  size_t len;

  list_foreach(&flow->modules, module)
  {
    len = strlen(module->name);
    if (strncmp(name, module->name, len) == 0)
      return module;
  }
  return NULL;
}

static flow_group_t *flow_lookup_group(flow_t *flow, const char *name)
{
  flow_group_t *group;

  list_foreach(&flow->groups, group)
    if (strcmp(group->name, name) == 0)
      return group;

  group = list_push_back(&flow->groups, NULL, sizeof *group);
  group->name = strdup(name);
  group->flow = flow;
  group->main_process = strcmp(group->name, "<main>") == 0;
  list_construct(&group->nodes);
  queue_producer_construct(&group->log_producer);
  if (!group->main_process)
    flow->log_consumer_sources++;
  return group;
}

static flow_node_t *flow_lookup_node(flow_t *flow, const char *name)
{
  flow_group_t *group;
  flow_node_t *node;

  list_foreach(&flow->groups, group)
  {
    list_foreach(&group->nodes, node)
      if (strcmp(node->name, name) == 0)
        return node;
  }
  return NULL;
}

static void flow_log_emit(flow_t *flow, value_t *message)
{
  reactor_call(&flow->user, FLOW_LOG, (uintptr_t) message);
}

static void flow_log_queue(reactor_event_t *event)
{
  flow_t *flow = event->state;
  value_t *message = *(value_t **) event->data;

  assert(event->type == QUEUE_CONSUMER_POP);
  if (!message)
  {
    flow->log_consumer_sources--;
    if (!flow->log_consumer_sources)
      queue_consumer_close(&flow->log_consumer);
  }
  else
  {
    flow_log_emit(flow, message);
    value_release(message);
  }
}

void flow_construct(flow_t *flow, reactor_callback_t *callback, void *state)
{
  int e;

  *flow = (flow_t) {.user = reactor_user_define(callback, state)};
  e = lt_dlinit();
  assert(e == 0);
  list_construct(&flow->modules);
  list_construct(&flow->groups);
  queue_construct(&flow->log_queue, sizeof (value_t *));
  queue_consumer_construct(&flow->log_consumer, flow_log_queue, flow);
}

void flow_destruct(flow_t *flow)
{
  queue_consumer_destruct(&flow->log_consumer);
  queue_destruct(&flow->log_queue);
  list_destruct(&flow->groups, flow_release_group);
  list_destruct(&flow->modules, flow_release_module);
  lt_dlexit();
}

void flow_search(flow_t *flow, const char *path)
{
  int e;

  (void) flow;
  e = lt_dlinsertsearchdir(lt_dlgetsearchpath(), path);
  assert(e == 0);
}

int flow_module(flow_t *flow, const char *module_name)
{
  flow_module_t *module = flow_lookup_module(flow, module_name);

  return module ? 0 : -1;
}

void flow_start(flow_t *flow)
{
  flow_module_t *module;
  flow_group_t *group;

  queue_consumer_open(&flow->log_consumer, &flow->log_queue, 1);
  if (flow->log_consumer_sources)
    queue_consumer_pop(&flow->log_consumer);

  list_foreach(&flow->modules, module)
    if (module->table->load)
      module->state = module->table->load();

  list_foreach(&flow->groups, group)
    flow_group_start(group);
}

void flow_stop(flow_t *flow)
{
  flow_group_t *group;
  flow_node_t *node;

  /* since main loop is ended all async groups should be gone by now */
  list_foreach(&flow->groups, group)
    if (group->main_process)
      list_foreach(&group->nodes, node)
        flow_node_stop(node);
}

int flow_node(flow_t *flow, const char *node_name, const char *module_name, const char *group_name, value_t *conf)
{
  flow_module_t *module;
  flow_group_t *group;
  flow_node_t *node;

  module = module_name ? flow_lookup_module(flow, module_name) : flow_match_module(flow, node_name);
  if (!module)
    return -1;

  group = flow_lookup_group(flow, group_name ? group_name : node_name);
  node = list_push_back(&group->nodes, NULL, sizeof *node);
  flow_node_construct(node, node_name, module, group, conf);
  return 0;
}

int flow_connect(flow_t *flow, const char *node1_name, const char *node2_name, value_t *conf)
{
  flow_node_t *node1, *node2;

  node1 = flow_lookup_node(flow, node1_name);
  node2 = flow_lookup_node(flow, node2_name);
  if (!node1 || !node2)
    return -1;

  flow_node_connect(node1, node2, conf);
  return 0;
}

void flow_send(flow_node_t *node, value_t *message)
{
  flow_node_send(node, message);
}

void flow_send_and_release(flow_node_t *node, value_t *message)
{
  flow_send(node, message);
  value_release(message);
}

void flow_log(flow_node_t *node, value_t *message)
{
  if (node->group->main_process)
    flow_log_emit(node->group->flow, message);
  else
  {
    value_hold(message);
    queue_producer_push(&node->group->log_producer, &message);
  }
}

void flow_exit(flow_node_t *node)
{
  flow_edge_t *edge;

  list_foreach(&node->edges, edge)
    if (!edge->direct)
      queue_producer_push(&edge->producer, (value_t *[]){NULL});
  flow_node_stop(node);
}
