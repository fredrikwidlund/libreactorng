#ifndef VALUE_DATA_H_INCLUDED
#define VALUE_DATA_H_INCLUDED

#include <stdlib.h>
#include <stdbool.h>
#include <sys/uio.h>

typedef struct data data_t;

struct data
{
  struct iovec iov;
};

/* constructor/destructor */

data_t  data(const void *, size_t);
data_t  data_null(void);
data_t  data_string(const char *);
data_t  data_offset(const data_t, size_t);
data_t  data_select(const data_t, size_t);
data_t  data_copy(const data_t);
data_t  data_copyz(const data_t);
data_t  data_alloc(size_t);
data_t  data_realloc(data_t, size_t);
void    data_release(data_t);

/* capacity */

size_t  data_size(const data_t);
bool    data_empty(const data_t);
bool    data_nullp(const data_t);

/* element access */

void   *data_base(const data_t);
void   *data_end(const data_t);

/* operations */

bool    data_equal(const data_t, const data_t);
bool    data_equal_case(const data_t, const data_t);

#endif /* VALUE_DATA_H_INCLUDED */
