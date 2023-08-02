#include <assert.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <reactor.h>

enum
{
  NETWORK_RESOLVE_TASK
};

typedef struct network_task         network_task_t;
typedef struct network_resolve_task network_resolve_task_t;
typedef struct network_cache        network_cache_t;

struct network_task
{
  reactor_user_t    user;
  void             *state;
  int               type;
};

struct network_resolve_task
{
  network_task_t    task;
  char             *host;
  reactor_id_t      next;
};

struct network_cache
{
  const char       *host;
  uint64_t          expire;
  struct addrinfo  *ai;
  reactor_id_t      async;
  list_t            requests;
};

struct network
{
  list_t   tasks;
  maps_t   cache;
  uint64_t cache_expiration;
};

static __thread struct network network = {.cache_expiration = 10000000000LL};

static void network_resolve_done(network_resolve_task_t *);

static void *network_task(reactor_callback_t *callback, void *state, int type)
{
  static const size_t sizes[] = {[NETWORK_RESOLVE_TASK] = sizeof (network_resolve_task_t)};
  network_task_t *task;

  task = list_push_back(&network.tasks, NULL, sizes[type]);
  reactor_user_construct(&task->user, callback, state);
  task->type = type;
  return task;
}

static void network_cache_release(maps_entry_t *entry)
{
  network_cache_t *cache = (network_cache_t *) entry->value;

  if (cache->ai)
    freeaddrinfo(cache->ai);
  list_destruct(&cache->requests, NULL);
}

void network_construct(void)
{
  list_construct(&network.tasks);
  maps_construct(&network.cache);
}

void network_destruct(void)
{
  list_destruct(&network.tasks, NULL);
  maps_destruct(&network.cache, network_cache_release);
}

void network_cancel(network_t id)
{
  network_task_t *task = (network_task_t *) id;

  network_resolve_done((network_resolve_task_t *) task);
}

void network_expire(uint64_t time)
{
  network.cache_expiration = time;
}

/* network resolve */

static bool network_resolve_is_numeric(const char *name)
{
  while (*name)
  {
    if (*name >= 'A')
      return false;
    name++;
  }
  return true;
}

static void network_resolve_done(network_resolve_task_t *task)
{
  free(task->host);
  if (task->next)
    reactor_cancel(task->next, NULL, NULL);
  list_erase(task, NULL);
}

static void network_resolve_numeric(network_resolve_task_t *task)
{
  struct addrinfo *ai = NULL;

  (void) getaddrinfo(task->host, NULL, (struct addrinfo []) {{
        .ai_flags = AI_NUMERICHOST | AI_NUMERICSERV,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM}},
    &ai);
  reactor_call(&task->task.user, NETWORK_RESOLVE, (uint64_t) ai);
  freeaddrinfo(ai);
  network_resolve_done(task);
}

static void network_resolve_async(reactor_event_t *event)
{
  network_cache_t *cache = event->state;
  reactor_user_t *user;

  switch (event->type)
  {
  case REACTOR_CALL:
    (void) getaddrinfo(cache->host, NULL, (struct addrinfo []) {{
          .ai_flags = AI_ADDRCONFIG,
          .ai_family = AF_UNSPEC,
          .ai_socktype = SOCK_STREAM}},
      &cache->ai);
    break;
  default:
    cache->async = 0;
    cache->expire = reactor_now() + network.cache_expiration;
    list_foreach(&cache->requests, user)
      reactor_call(user, NETWORK_RESOLVE, (uint64_t) cache->ai);
    list_clear(&cache->requests, NULL);
  }
}

static void network_resolve_next(reactor_event_t *event)
{
  network_resolve_task_t *task = event->state;
  network_cache_t *cache;

  task->next = 0;
  if (network_resolve_is_numeric(task->host))
  {
    network_resolve_numeric(task);
    return;
  }

  cache = (network_cache_t *) maps_at(&network.cache, task->host);
  if (!cache)
  {
    cache = calloc(1, sizeof *cache);
    cache->host = task->host;
    list_construct(&cache->requests);
    maps_insert(&network.cache, cache->host, (uintptr_t) cache, NULL);
    task->host = NULL;
  }

  if (cache->ai)
  {
    if (reactor_now() < cache->expire)
    {
      reactor_call(&task->task.user, NETWORK_RESOLVE, (uint64_t) cache->ai);
      network_resolve_done(task);
      return;
    }
    freeaddrinfo(cache->ai);
    cache->ai = NULL;
    cache->expire = 0;
  }

  list_push_back(&cache->requests, &task->task.user, sizeof task->task.user);
  if (!cache->async)
    cache->async = reactor_async(network_resolve_async, cache);
  network_resolve_done(task);
}

network_t network_resolve(reactor_callback_t *callback, void *state, const char *host)
{
  network_resolve_task_t *task = network_task(callback, state, NETWORK_RESOLVE_TASK);

  task->host = strdup(host);
  task->next = reactor_next(network_resolve_next, task);
  return (network_t) task;
}
