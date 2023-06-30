# libreactor

## Data types

### Data vectors

Data vectors are static data containers reducing the need for the common use case of handling separate variables for pointers and size. Data vectors also remove the need for zero-terminated strings, and help reduce strlen() runtime usage.

#### Example

```C
#include <unistd.h>
#include <reactor.h>

void out(data_t d)
{
  write(1, data_base(d), data_size(d));
}

int main()
{
  data_t d = data_string("compiled with \"-flto -O2\" this application will not call strlen() later down the call stack\n");
  out(d);
}                                                                                                                                                    ```
```

### Buffers

Buffers offers generic data containers with dynamic memory allocation. Buffers can inserted into, erased from, resized and compacted, saved to and loaded from files.

#### Example

```C
#include <reactor.h>

int main()
{
  buffer_t b;

  buffer_construct(&b);
  buffer_load(&b, "/etc/motd");
  buffer_prepend(&b, data_string("--- first line ---\n"));
  buffer_append(&b, data_string("--- last line ---\n"));
  buffer_save(&b, "/tmp/motd");
  buffer_destruct(&b);
}
```
