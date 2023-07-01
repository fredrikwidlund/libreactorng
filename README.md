![CodeQL](https://github.com/fredrikwidlund/libreactorng/actions/workflows/codeql.yml/badge.svg)
![Build](https://github.com/fredrikwidlund/libreactorng/actions/workflows/c-cpp.yml/badge.svg)

# ![](https://github.com/fredrikwidlund/libreactorng/assets/2116262/ecf3a17c-86fd-4dcc-909c-0f99a24466c9) libreactor

## About

libreactor is a [high performance](#performance), [robust and secure](#security), generic event-driven application framework for Linux. The design goal is to minimize software overhead, cpu usage, energy consumption and environmental footprint. libreactor is directly built on top of the [Linux kernel io_uring](https://kernel.dk/io_uring.pdf) system call interface, offering both much simplified access to low level asynchronous kernel calls, as well as high level event-driven abstractions such as HTTP servers. Furthermore libreactor is built completely without third-party dependencies, minimizing supply chain risk.

## Key Features

- Data types such as [data vectors](#data-vectors), [buffers](#buffers), lists, hash tables, dynamic arrays, UTF-8 strings, JSON values (including RFC 8259 compliant serialization)
- Low level io_uring abstrations
- High level event abstrations
- Message queues
- Declarative graph based data flow application framework

## Performance

The current version of libreactor is completely refactored to use the Linux kernel io_uring system call interface, achieving a clear performance jump from the previous epoll()-based version that has been a top contender of the [Techempower benchmark](https://www.techempower.com/benchmarks/#section=data-r21&test=json) for many years (the benchmark is in itself flawed in many ways but should still give an indication of performance potential).

A fast framework can help you achieve performance, but it can not replace the need for optimized software design and architecture. If your software design is flawed from a performance perspective you will not achieve high performance and high availability, regardless of implementation details.

## Security

The libreactor pipeline is built with unit-tests that require 100% line coverage, and 100% branch coverage (the latter probably in itself indicates an OCD-diagnosis) to succeed. The above tests are also completed using Valgrind to ensure memory management hygiene. Static analysis is done using CodeQL.

Although this is a refactored version of libreactor, the previous version has been running in production as web API servers 24/7/365 for almost 10 years serving many millions of unique end-users, at consistently very high RPS rates, with perfect availability. Since there are few moving parts, no real dependencies, and the kernel API is very stable (perhaps less so with io_uring) change management is typically very simple. 

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
