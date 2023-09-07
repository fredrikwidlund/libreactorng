#ifndef REACTOR_FLOW_H_INCLUDED
#define REACTOR_FLOW_H_INCLUDED

enum
{
  FLOW_ERROR,
  FLOW_LOG
};

typedef struct flow_table  flow_table_t;
typedef struct flow_module flow_module_t;
typedef struct flow_group  flow_group_t;
typedef struct flow_node   flow_node_t;
typedef struct flow_edge   flow_edge_t;
typedef struct flow        flow_t;

struct flow_table
{
  void               *(*load)(void);
  void               *(*create)(flow_node_t *, value_t *);
  void                (*receive)(void *, value_t *);
  void                (*destroy)(void *);
  void                (*unload)(void *);
};

struct flow_module
{
  const char            *name;
  const void            *handle;
  const flow_table_t    *table;
  void                  *state;
};

struct flow_group
{
  const char            *name;
  flow_t                *flow;
  bool                   main_process;
  reactor_t              async;
  list_t                 nodes;
  queue_producer_t       log_producer;
};

struct flow_node
{
  const char            *name;
  const flow_table_t    *table;
  flow_group_t          *group;
  value_t               *conf;
  bool                   started;
  list_t                 edges;
  queue_t                queue;
  size_t                 consumer_sources;
  queue_consumer_t       consumer;
  void                  *state;
};

struct flow_edge
{
  bool                   direct;
  flow_node_t           *target;
  value_t               *conf;
  queue_producer_t       producer;
};

struct flow
{
  reactor_user_t         user;
  list_t                 modules;
  list_t                 groups;
  // value_t *globals;
  queue_t                log_queue;
  size_t                 log_consumer_sources;
  queue_consumer_t       log_consumer;
};

/* flow functions */

void flow_construct(flow_t *, reactor_callback_t *, void *);
void flow_destruct(flow_t *);
void flow_search(flow_t *, const char *);
int  flow_module(flow_t *, const char *);
int  flow_node(flow_t *, const char *, const char *, const char *, value_t *);
int  flow_connect(flow_t *, const char *, const char *, value_t *);
void flow_start(flow_t *);
void flow_stop(flow_t *);

/* flow node functions */

void flow_send(flow_node_t *, value_t *);
void flow_send_and_release(flow_node_t *, value_t *);
void flow_log(flow_node_t *, value_t *);
void flow_exit(flow_node_t *);

#endif /* REACTOR_FLOW_H_INCLUDED */
