#ifndef VALUE_H
#define VALUE_H

#ifdef UNIT_TESTING
extern void mock_assert(const int, const char* const, const char * const, const int);
#undef assert
#define assert(expression) \
    mock_assert((int)(expression), #expression, __FILE__, __LINE__);
#endif

#define VALUE_VERSION       "1.0.0-alpha"
#define VALUE_VERSION_MAJOR 1
#define VALUE_VERSION_MINOR 0
#define VALUE_VERSION_PATCH 0

#define value_likely(x)   __builtin_expect(!!(x), 1)
#define value_unlikely(x) __builtin_expect(!!(x), 0)

#ifdef __cplusplus
extern "C" {
#endif

#include "value/data.h"
#include "value/hash.h"
#include "value/buffer.h"
#include "value/list.h"
#include "value/vector.h"
#include "value/map.h"
#include "value/mapi.h"
#include "value/maps.h"
#include "value/mapd.h"
#include "value/string.h"
#include "value/value.h"
#include "value/encode.h"
#include "value/decode.h"

#ifdef __cplusplus
}
#endif

#endif /* VALUE_H */
