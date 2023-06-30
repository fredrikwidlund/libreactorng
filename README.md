![CodeQL](https://github.com/fredrikwidlund/libreactorng/actions/workflows/codeql.yml/badge.svg)
![Build](https://github.com/fredrikwidlund/libreactorng/actions/workflows/c-cpp.yml/badge.svg)

# ![](https://github.com/fredrikwidlund/libreactorng/assets/2116262/ecf3a17c-86fd-4dcc-909c-0f99a24466c9) libreactor

## About

libreactor is a [high performance](#performance), robust and secure, generic event-driven application framework for Linux. The design goal is to minimize software overhead, cpu usage, energy consumption and environmental footprint. libreactor is directly built on top of the [Linux kernel io_uring](https://kernel.dk/io_uring.pdf) system call interface, offering both much simplified access to low level asynchronous kernel calls, as well as high level event-driven abstractions such as HTTP servers. Furthermore libreactor is built completely without third-party dependencies, minimizing supply chain risk.

## Key Features

- Data types such as [buffers](doc/types.md#buffers), lists, hash tables, vectors, utf8 encoded strings, JSON values (including RFC 8259 compliant serialization)
- Low level io_uring abstrations
- High level event abstrations
- Message queues
- Declarative graph based data flow
- and more...

## Performance

...

## Security

...
