# libreactor

## Data types

### Buffers

libreactor buffers offers generic data containers with dynamic memory allocation. Buffers can inserted into, erased from, resized and compacted, saved to and loaded from files.

#### Example

```C
#include <reactor.h>

int main()
{
  buffer_t b;

  buffer_construct(&b);
  buffer_load(&b, "/etc/motd");
  buffer_append(&b, data_string("add some noise...\n"));
  buffer_save(&b, "/tmp/motd");
  buffer_destruct(&b);
}
```
