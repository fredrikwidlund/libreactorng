#ifndef REACTOR_H_INCLUDED
#define REACTOR_H_INCLUDED

#ifdef UNIT_TESTING
extern void mock_assert(const int, const char* const, const char * const, const int);
#undef assert
#define assert(expression) \
    mock_assert((int)(expression), #expression, __FILE__, __LINE__);
#endif

#define REACTOR_VERSION       "1.0.0-alpha"
#define REACTOR_VERSION_MAJOR 1
#define REACTOR_VERSION_MINOR 0
#define REACTOR_VERSION_PATCH 0

#define reactor_likely(x)   __builtin_expect(!!(x), 1)
#define reactor_unlikely(x) __builtin_expect(!!(x), 0)

#ifdef __cplusplus
extern "C" {
#endif

#include "reactor/data.h"
#include "reactor/hash.h"
#include "reactor/buffer.h"
#include "reactor/list.h"
#include "reactor/vector.h"
#include "reactor/map.h"
#include "reactor/mapi.h"
#include "reactor/maps.h"
#include "reactor/mapd.h"
#include "reactor/string.h"
#include "reactor/value.h"
#include "reactor/encode.h"
#include "reactor/decode.h"
#include "reactor/pool.h"
#include "reactor/reactor.h"
#include "reactor/event.h"

#ifdef __cplusplus
}
#endif

#endif /* REACTOR_H_INCLUDED */
