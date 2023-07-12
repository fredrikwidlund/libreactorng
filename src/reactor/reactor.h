#ifndef REACTOR_REACTOR_H
#define REACTOR_REACTOR_H

#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <reactor.h>
#include <linux/version.h>

#define REACTOR_RING_SIZE 4096

enum
{
  REACTOR_CALL,
  REACTOR_RETURN
};

typedef struct reactor_event  reactor_event_t;
typedef struct reactor_user   reactor_user_t; //
typedef uint64_t              reactor_time_t;
typedef uint64_t              reactor_id_t;
typedef void                 (reactor_callback_t)(reactor_event_t *);

struct reactor_event
{
  void               *state;
  int                 type;
  uint64_t            data;
};

struct reactor_user
{
  reactor_callback_t *callback;
  void               *state;
};

reactor_event_t reactor_event_define(void *, int, uint64_t);
reactor_user_t  reactor_user_define(reactor_callback_t *, void *);
void            reactor_user_construct(reactor_user_t *, reactor_callback_t *, void *);

void            reactor_construct(void);
void            reactor_destruct(void);
reactor_time_t  reactor_now(void);
void            reactor_loop(void);
void            reactor_loop_once(void);
void            reactor_call(reactor_user_t *, int, uint64_t);
void            reactor_cancel(reactor_id_t, reactor_callback_t *, void *);

reactor_id_t    reactor_async(reactor_callback_t *, void *);
reactor_id_t    reactor_next(reactor_callback_t *, void *);
reactor_id_t    reactor_async_cancel(reactor_callback_t *, void *, uint64_t);
reactor_id_t    reactor_nop(reactor_callback_t *, void *);
reactor_id_t    reactor_readv(reactor_callback_t *, void *, int, const struct iovec *, int, size_t);
reactor_id_t    reactor_writev(reactor_callback_t *, void *, int, const struct iovec *, int, size_t);
reactor_id_t    reactor_fsync(reactor_callback_t *, void *, int);
reactor_id_t    reactor_poll_add(reactor_callback_t *, void *, int, short int);
reactor_id_t    reactor_poll_add_multi(reactor_callback_t *, void *, int, short int);
reactor_id_t    reactor_poll_update(reactor_callback_t *, void *, reactor_id_t, short int);
reactor_id_t    reactor_poll_remove(reactor_callback_t *, void *, reactor_id_t);
reactor_id_t    reactor_epoll_ctl(reactor_callback_t *, void *, int, int, int, struct epoll_event *);
reactor_id_t    reactor_sync_file_range(reactor_callback_t *, void *, int, uint64_t, uint64_t, int);
reactor_id_t    reactor_sendmsg(reactor_callback_t *, void *, int, const struct msghdr *, int);
reactor_id_t    reactor_recvmsg(reactor_callback_t *, void *, int, struct msghdr *, int);
reactor_id_t    reactor_send(reactor_callback_t *, void *, int, const void *, size_t, int);
reactor_id_t    reactor_recv(reactor_callback_t *, void *, int, void *, size_t, int);
reactor_id_t    reactor_timeout(reactor_callback_t *, void *, struct timespec *, int, int);
reactor_id_t    reactor_accept(reactor_callback_t *, void *, int, struct sockaddr *, socklen_t *, int);
reactor_id_t    reactor_read(reactor_callback_t *, void *, int, void *, size_t, size_t);
reactor_id_t    reactor_write(reactor_callback_t *, void *, int, const void *, size_t, size_t);
reactor_id_t    reactor_connect(reactor_callback_t *, void *, int, struct sockaddr *, socklen_t);
reactor_id_t    reactor_fallocate(reactor_callback_t *, void *, int, int, uint64_t, uint64_t);
/* fadvise */
/* madvise */
/* ... */
reactor_id_t    reactor_close(reactor_callback_t *, void *, int);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
reactor_id_t    reactor_send_zerocopy(reactor_callback_t *, void *, int, const void *, size_t, int);
#endif

#endif /* REACTOR_REACTOR_H */
