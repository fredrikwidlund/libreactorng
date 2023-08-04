#include <assert.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <reactor.h>

enum
{
  NETWORK_RESOLVE_TASK,
  NETWORK_ACCEPT_TASK,
  NETWORK_CONNECT_TASK
};

typedef struct network_task         network_task_t;
typedef struct network_resolve_task network_resolve_task_t;
typedef struct network_accept_task  network_accept_task_t;
typedef struct network_connect_task network_connect_task_t;
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
  reactor_t         next;
};

struct network_accept_task
{
  network_task_t    task;
  int               port;
  int               flags;
  network_t         resolve;
  int               fd;
  reactor_t         accept;
};

struct network_connect_task
{
  network_task_t    task;
  int               port;
  network_t         resolve;
  int               fd;
  reactor_t         connect;
  struct sockaddr  *addr;
  socklen_t         addrlen;
};

struct network_cache
{
  const char       *host;
  uint64_t          expire;
  struct addrinfo  *ai;
  reactor_t         async;
  list_t            requests;
};

struct network
{
  list_t   tasks;
  maps_t   cache;
  uint64_t cache_expiration;
};

static __thread struct network network = {.cache_expiration = 10000000000LL};

static void network_resolve_task_done(network_resolve_task_t *);
static void network_accept_task_done(network_accept_task_t *);
static void network_connect_task_done(network_connect_task_t *);

static void *network_task(reactor_callback_t *callback, void *state, int type)
{
  static const size_t sizes[] = {
    [NETWORK_RESOLVE_TASK] = sizeof (network_resolve_task_t),
    [NETWORK_ACCEPT_TASK] = sizeof (network_accept_task_t),
    [NETWORK_CONNECT_TASK] = sizeof (network_connect_task_t)
  };
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

  if (task->type == NETWORK_RESOLVE_TASK)
    network_resolve_task_done((network_resolve_task_t *) task);
  else if (task->type == NETWORK_ACCEPT_TASK)
    network_accept_task_done((network_accept_task_t *) task);
  else
  {
    assert(task->type == NETWORK_CONNECT_TASK);
    network_connect_task_done((network_connect_task_t *) task);
  }
}

void network_expire(uint64_t time)
{
  network.cache_expiration = time;
}

/* network error */

static void network_error(void *task)
{
  reactor_call(&((network_task_t *) task)->user, NETWORK_ERROR, 0);
  network_cancel((network_t) task);
}

/* network resolve */

static bool network_resolve_task_is_numeric(const char *name)
{
  if (!name)
    return false;
  while (*name)
  {
    if (*name >= 'A')
      return false;
    name++;
  }
  return true;
}

static void network_resolve_task_done(network_resolve_task_t *task)
{
  free(task->host);
  if (task->next)
    reactor_cancel(task->next, NULL, NULL);
  list_erase(task, NULL);
}

static void network_resolve_task_numeric(network_resolve_task_t *task)
{
  struct addrinfo *ai = NULL;

  (void) getaddrinfo(task->host, NULL, (struct addrinfo []) {{
        .ai_flags = AI_NUMERICHOST | AI_NUMERICSERV,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM}},
    &ai);
  reactor_call(&task->task.user, NETWORK_RESOLVE, (uint64_t) ai);
  freeaddrinfo(ai);
  network_resolve_task_done(task);
}

static void network_resolve_task_async(reactor_event_t *event)
{
  network_cache_t *cache = event->state;
  reactor_user_t *user;

  switch (event->type)
  {
  case REACTOR_CALL:
    (void) getaddrinfo(cache->host, cache->host ? NULL : "0", (struct addrinfo []) {{
          .ai_flags = AI_ADDRCONFIG | AI_PASSIVE,
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

static void network_resolve_task_next(reactor_event_t *event)
{
  network_resolve_task_t *task = event->state;
  network_cache_t *cache;
  const char *key;

  task->next = 0;
  if (network_resolve_task_is_numeric(task->host))
  {
    network_resolve_task_numeric(task);
    return;
  }

  key = task->host ? task->host : "<null>";
  cache = (network_cache_t *) maps_at(&network.cache, key);
  if (!cache)
  {
    cache = calloc(1, sizeof *cache);
    cache->host = task->host;
    list_construct(&cache->requests);
    maps_insert(&network.cache, key, (uintptr_t) cache, NULL);
    task->host = NULL;
  }

  if (cache->ai)
  {
    if (reactor_now() < cache->expire)
    {
      reactor_call(&task->task.user, NETWORK_RESOLVE, (uint64_t) cache->ai);
      network_resolve_task_done(task);
      return;
    }
    freeaddrinfo(cache->ai);
    cache->ai = NULL;
    cache->expire = 0;
  }

  list_push_back(&cache->requests, &task->task.user, sizeof task->task.user);
  if (!cache->async)
    cache->async = reactor_async(network_resolve_task_async, cache);
  network_resolve_task_done(task);
}

network_t network_resolve(reactor_callback_t *callback, void *state, const char *host)
{
  network_resolve_task_t *task = network_task(callback, state, NETWORK_RESOLVE_TASK);

  if (host)
    task->host = strdup(host);
  task->next = reactor_next(network_resolve_task_next, task);
  return (network_t) task;
}

/* network accept */

static void network_accept_task_callback(reactor_event_t *);

static void network_accept_task_done(network_accept_task_t *task)
{
  if (task->resolve)
    network_cancel(task->resolve);
  if (task->fd >= 0)
    (void) reactor_close(NULL, NULL, task->fd);
  if (task->accept)
    reactor_cancel(task->accept, NULL, NULL);
  list_erase(task, NULL);
}

static void network_accept_task_accept(network_accept_task_t *task)
{
  task->accept = reactor_accept(network_accept_task_callback, task, task->fd, NULL, NULL, SOCK_CLOEXEC);
}

static void network_accept_task_callback(reactor_event_t *event)
{
  network_accept_task_t *task = event->state;
  int result = event->data;

  task->accept = 0;
  if (result < 0)
  {
    network_error(task);
    return;
  }
  network_accept_task_accept(task);
  reactor_call(&task->task.user, NETWORK_ACCEPT, result);
}

static void network_accept_task_socket(network_accept_task_t *task, struct sockaddr *addr, socklen_t addrlen)
{
  struct sockaddr *addr_copy;
  int e;

  task->fd = socket(addr->sa_family, SOCK_STREAM, 0);
  if (task->fd == -1)
  {
    network_error(task);
    return;
  }

  if (task->flags & NETWORK_REUSEADDR)
    (void) setsockopt(task->fd, SOL_SOCKET, SO_REUSEADDR, (int[]) {1}, sizeof(int));
  if (task->flags & NETWORK_REUSEPORT)
    (void) setsockopt(task->fd, SOL_SOCKET, SO_REUSEPORT, (int[]) {1}, sizeof(int));

  addr_copy = malloc(addrlen);
  memcpy(addr_copy, addr, addrlen);
  if (task->port)
  {
    if (addr->sa_family == AF_INET)
      ((struct sockaddr_in *) addr)->sin_port = htons(task->port);
    else
    {
      assert(addr->sa_family == AF_INET6);
      ((struct sockaddr_in6 *) addr)->sin6_port = htons(task->port);
    }
  }
  e = bind(task->fd, addr_copy, addrlen);
  free(addr_copy);
  if (e == -1)
  {
    network_error(task);
    return;
  }

  (void) listen(task->fd, INT_MAX);
  network_accept_task_accept(task);
  reactor_call(&task->task.user, NETWORK_ACCEPT_BIND, task->fd);
}

static void network_accept_resolve(reactor_event_t *event)
{
  network_accept_task_t *task = event->state;
  struct addrinfo *ai = (struct addrinfo *) event->data;

  task->resolve = 0;
  if (!ai)
  {
    network_error(task);
    return;
  }
  network_accept_task_socket(task, ai->ai_addr, ai->ai_addrlen);
}

network_t network_accept(reactor_callback_t *callback, void *state, const char *host, int port, int flags)
{
  network_accept_task_t *task = network_task(callback, state, NETWORK_ACCEPT_TASK);

  task->port = port;
  task->fd = -1;
  task->flags = flags;
  task->resolve = network_resolve(network_accept_resolve, task, host);
  return (network_t) task;
}

network_t network_accept_address(reactor_callback_t *callback, void *state, struct sockaddr *addr, socklen_t addrlen, int flags)
{
  network_accept_task_t *task = network_task(callback, state, NETWORK_ACCEPT_TASK);

  task->fd = -1;
  task->flags = flags;
  network_accept_task_socket(task, addr, addrlen);
  return (network_t) task;
}

network_t network_accept_socket(reactor_callback_t *callback, void *state, int fd)
{
  network_accept_task_t *task = network_task(callback, state, NETWORK_ACCEPT_TASK);

  task->fd = fd;
  network_accept_task_accept(task);
  return (network_t) task;
}

/* network connect */

static void network_connect_task_done(network_connect_task_t *task)
{
  if (task->resolve)
    network_cancel(task->resolve);
  if (task->fd >= 0)
    (void) reactor_close(NULL, NULL, task->fd);
  if (task->connect)
    reactor_cancel(task->connect, NULL, NULL);
  free(task->addr);
  list_erase(task, NULL);
}

static void network_connect_task_callback(reactor_event_t *event)
{
  network_connect_task_t *task = event->state;
  int result = event->data;

  task->connect = 0;
  if (result < 0)
  {
    network_error(task);
    return;
  }
  reactor_call(&task->task.user, NETWORK_CONNECT, task->fd);
  task->fd = -1;
  network_connect_task_done(task);
}

static void network_connect_task_socket(network_connect_task_t *task, struct sockaddr *addr, socklen_t addrlen)
{
  task->fd = socket(addr->sa_family, SOCK_STREAM, 0);
  if (task->fd == -1)
  {
    network_error(task);
    return;
  }

  task->addr = malloc(addrlen);
  task->addrlen = addrlen;
  memcpy(task->addr, addr, addrlen);
  if (task->port)
  {
    if (addr->sa_family == AF_INET)
      ((struct sockaddr_in *) task->addr)->sin_port = htons(task->port);
    else
    {
      assert(addr->sa_family == AF_INET6);
      ((struct sockaddr_in6 *) task->addr)->sin6_port = htons(task->port);
    }
  }
  task->connect = reactor_connect(network_connect_task_callback, task, task->fd, task->addr, task->addrlen);
}

static void network_connect_resolve(reactor_event_t *event)
{
  network_connect_task_t *task = event->state;
  struct addrinfo *ai = (struct addrinfo *) event->data;

  task->resolve = 0;
  if (!ai)
  {
    network_error(task);
    return;
  }
  network_connect_task_socket(task, ai->ai_addr, ai->ai_addrlen);
}

network_t network_connect(reactor_callback_t *callback, void *state, const char *host, int port)
{
  network_accept_task_t *task = network_task(callback, state, NETWORK_CONNECT_TASK);

  task->port = port;
  task->fd = -1;
  task->resolve = network_resolve(network_connect_resolve, task, host);
  return (network_t) task;
}

network_t network_connect_address(reactor_callback_t *callback, void *state, struct sockaddr *addr, socklen_t addrlen)
{
  network_connect_task_t *task = network_task(callback, state, NETWORK_CONNECT_TASK);

  task->fd = -1;
  network_connect_task_socket(task, addr, addrlen);
  return (network_t) task;
}

