![CodeQL](https://github.com/fredrikwidlund/libreactorng/actions/workflows/codeql.yml/badge.svg)
![Build](https://github.com/fredrikwidlund/libreactorng/actions/workflows/c-cpp.yml/badge.svg)

# ![](https://github.com/fredrikwidlund/libreactorng/assets/2116262/834b278c-23e2-4688-a312-23ba965dba01) libreactor

## About

libreactor is a [high performance](#performance), [robust and secure](#security), generic event-driven application framework for Linux. The design goal is to minimize software overhead, cpu usage, energy consumption and environmental footprint. libreactor is directly built on top of the [Linux kernel io_uring](https://kernel.dk/io_uring.pdf) system call interface, offering both much simplified access to low level asynchronous kernel calls, as well as high level event-driven abstractions such as HTTP servers. Furthermore libreactor is built completely without third-party dependencies, minimizing supply chain risk.

## Key Features

- Data types such as [data vectors](#data-vectors), [buffers](#buffers), [lists](#lists), [dynamic arrays](#vectors), [hash tables](#maps), [UTF-8 strings](#utf-8-strings), [JSON values (including RFC 8259 compliant serialization)](#json), [memory pools](#memory-pools)
- Generic [event driven framework](#events)
- Support for [threads](#threads)
- Low level io_uring abstrations
- High level event abstrations
- Message queues
- Declarative graph based data flow application framework

## Performance

The current version of libreactor is completely refactored to use the Linux kernel io_uring system call interface, achieving a clear performance jump from the previous epoll()-based version that has been a top contender of the [Techempower benchmark](https://www.techempower.com/benchmarks/#section=data-r21&test=json) for many years (the benchmark is in itself flawed in many ways but should still give an indication of performance potential).

A fast framework can help you achieve performance, but it can not replace the need for optimized software design and architecture. If your software design is flawed from a performance perspective you will not achieve high performance and high availability, regardless of implementation details.

## Security

The libreactor pipeline is built with unit-tests that require 100% line coverage, and 100% branch coverage (the latter probably in itself indicates an OCD-diagnosis) to succeed. The above tests are also completed using Valgrind to ensure memory management hygiene. Static analysis is done using CodeQL.

Although this is a refactored version of libreactor, the previous version has been running in production as web API servers 24/7/365 for almost 10 years serving many millions of unique end-users, at consistently very high RPS rates, actually with perfect availability so far. Since there are few moving parts, no real third-party dependencies, and the kernel API is very stable (perhaps less so with io_uring) change management is typically very simple. 

## Installation

``` sh
git clone https://github.com/fredrikwidlund/libreactorng.git
cd libreactorng/
./autogen.sh
./configure
make && sudo make install
```

## Usage

```sh
cat > main.c <<EOF
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
EOF
gcc -Wall -o main main.c `pkg-config --libs --cflags libreactor`
./main
```

## Data types

### Data vectors

Data vectors are generic data containers reducing the need for the common use case of handling separate variables for pointers and size. Data vectors also remove the need for zero-terminated strings, and help reduce strlen() runtime usage.

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

### Lists

Lists are doubly linked sequence containers, similar to C++ std::list, with O(1) inserts (given a known position) and deletes.

#### Example

Naive example printing primes up to n (10000).

```C
#include <reactor.h>

int main()
{
  list_t l;
  int i, n = 10000, *p_first, *p, *p_next;

  list_construct(&l);
  /* add all integers to n */
  for (i = 1; i < n; i++)
    list_push_back(&l, &i, sizeof i);
  /* skip first integer 1 */
  p_first = list_next(list_front(&l));
  /* remove all multiples of first prime in list */
  while (p_first != list_end(&l) && *p_first <= n / 2)
  {
    p = list_next(p_first);
    while (p != list_end(&l))
    {
      p_next = list_next(p);
      if (*p % *p_first == 0)
        list_erase(p, NULL);
      p = p_next;
    }
    p_first = list_next(p_first);
  }
  /* print remaining integers */
  list_foreach(&l, p)
    printf("%d\n", *p);
  list_destruct(&l, NULL);
}
```

### Vectors

Vectors are dynamically resized arrays, similar to C++ std::vector, with O(1) random access, and O(1) inserts and removals at the end.

#### Example

Create a vector with the first 50 Fibonacci integers.

```C
#include <reactor.h>

void fib(vector_t *v, size_t a, size_t b, int remaining)
{
  if (!remaining)
    return;
  vector_push_back(v, &a);
  fib(v, b, a + b, remaining - 1);
}

int main()
{
  vector_t v;
  int i;

  vector_construct(&v, sizeof (size_t));
  fib(&v, 0, 1, 50);
  for (i = 0; i < vector_size(&v); i++)
    printf("%lu\n", *(size_t *) vector_at(&v, i));
  vector_destruct(&v, NULL);
}
```

### Maps

Maps are associative containers that contains key-value pairs with unique keys, similar to std::unordered_map, with average O(1) random access, inserts, and removals.

Maps come in a generic map_t flavour, as well as mapi_t for integer keys and maps_t for string keys.

#### Example

Constant lookups against a Fibonacci table.

```C
#include <reactor.h>

void fib(mapi_t *m, size_t a, size_t b, int remaining)
{
  if (!remaining)
    return;
  mapi_insert(m, a, 1, NULL);
  fib(m, b, a + b, remaining - 1);
}

int main()
{
  mapi_t m;
  size_t test[] = {233, 234, 377, 378, 610, 611};
  int i;

  mapi_construct(&m);
  fib(&m, 0, 1, 50);
  for (i = 0; i < sizeof test / sizeof *test; i++)
    printf("%lu %s a Fibonacci number\n", test[i], mapi_at(&m, test[i]) ? "is" : "is not");
  mapi_destruct(&m, NULL);
}
```

### UTF-8 strings

UTF-8 compliant strings store an explicit string length to avoid zero termination issues.

#### Example

Prints a simple unicode string including the zero character.

```C
int main()
{
  string_t s = string_utf8_decode("unicode character \\ud83d\\ude03, zero character \\u0000, some more text...\\n", NULL);

  printf("string length: %lu\n", string_utf8_length(s));
  fwrite(string_base(s), string_size(s), 1, stdout);
}
```

### JSON

JSON support includes an opaque complex value_t container that can dynamic generic data structures runtime, as well as serialize to, and deserialize from fully compliant JSON notation. The value_t type is extendable to support non standard JSON values such as binary data and references. 

#### Example

Creates and serializes a simple value_t object.

```C
int main()
{
  value_t *v, *k;
  string_t s;

  v = value_object();
  value_object_set_release(v, string("a smiley"), value_string(string("\U0001F600")));
  k = value_array();
  value_object_set_release(v, string("some interesting numbers"), k);
  value_array_append_release(k, value_number(184467440737095516.0L));
  value_array_append_release(k, value_number(123.456e-789L));
  value_array_append_release(k, value_number(3.14L));

  s = value_encode(v, 1);
  fwrite(string_base(s), string_size(s), 1, stdout);
  string_release(s);

  value_release(v);
}
```

### Memory pools

Memory pools improves performance when frequently allocating and deallocating objects of a predetermined size.

#### Example

Allocates an integer and returns it to the pool.

```C
int main()
{
  pool_t p;
  int *x;

  pool_construct(&p, sizeof (int));
  x = pool_malloc(&p);
  *x = 42;
  pool_free(&p, x);
  pool_destruct(&p);
}
```

### Events

Event driven architecture is a core design pattern of a libreactor application and implies using asyncronous operations to avoid blocking.

The simplest possible (do nothing) application is shown below. It will initialize the reactor, process all events, and then release all reasources required for the reactor. Since there is nothing triggering events it will terminate without side expressions.

```C
int main()
{
  reactor_construct();
  reactor_loop();
  reactor_destruct();
}
```

An event is a value created asyncronously by a producer, for example as a result of a asyncronous kernel syscall, or a high order abstract object.

To define how to receive events a *callback* and a *state* is typically prefixed to a low level syscall, or as arguments when constructing an object. The *state* and *data* are sent as part of the resulting *event* (`event->state` and `event->data`). Below is a simple application that asyncronously will read from *stdin* into a *buffer*. When the *reactor_read* command is completed the loop will exit.

```C
void callback(reactor_event_t *event)
{
  printf("read returned %d\n", (int) event->data);
}

int main()
{
  char buffer[1024];

  reactor_construct();
  reactor_read(callback, NULL, 0, buffer, sizeof buffer, 0);
  reactor_loop();
  reactor_destruct();
}
```

The power of the event driven pattern is that actors can operate concurrently in the same thread, with a shared state, without a need for locks, mutexes and semaphores, and without a need for context switching. This allows for highly scalable and performance optimized applications, while also minimizing the risk for race conditions. 

Event driven applications tend to be `flexible, loosely-coupled and scalable`, `easier to develop and amenable to change` and `significantly more tolerant of failure`.

### Threads

Threads can be spawned and results collected using an event driven pattern.

#### Example

```C
void async_callback(reactor_event_t *event)
{
  if (event->type == REACTOR_CALL)
  {
    printf("separate thread that can block and manipulate state\n");
    sleep(1);
    printf("exit\n");
  }
}

int main()
{
  reactor_construct();
  reactor_async(async_callback, NULL);
  reactor_loop();
  reactor_destruct();
}
```
