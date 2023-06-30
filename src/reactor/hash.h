#ifndef VALUE_HASH_H_INCLUDED
#define VALUE_HASH_H_INCLUDED

#include <stdint.h>
#include <reactor.h>

uint64_t hash_data(const data_t);
uint64_t hash_uint64(uint64_t);

#endif /* VALUE_HASH_H_INCLUDED */
