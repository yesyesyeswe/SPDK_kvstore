# [SPDK_kvstore](https://github.com/yesyesyeswe/SPDK_kvstore)

A high-performance KV storage system based on SPDK, supporting multi-engine storage architectures and custom memory management.

## Features

- **Storage Engine Module**: Designed and implemented a storage engine abstraction layer supporting three underlying implementations: arrays, RB trees, and hash tables. Distributes requests to different storage engines via command prefixes (e.g., RGET/RDEL), enabling flexible extension of new storage engines.
- **Network Service Module**: Implements zero-copy data transfer based on the SPDK framework, utilizing an event-driven model to handle concurrent requests.
- **Memory Management Module**: Independently implements a high-performance memory allocator (mymalloc) with a 4KB management granularity and dynamic partitioning. Supports 8-byte alignment by default (configurable) and automatic memory merging.

## Prerequisites

- Linux 6.2+
- Spdk 23.05+

## Installation

```bash
git clone https://github.com/yesyesyeswe/SPDK_kvstore.git
cd SPDK_kvstore
make
```

## Usage

```bash
./kvstore  -H 0.0.0.0 -P 8888 -N posix
```

## Project Structure

```bash
Makefile
src
├── engine
│   ├── kv_array.c
│   ├── kv_hash.c
│   └── kv_rbtree.c
├── kvstore.c
├── kvstore.h
├── mm
│   ├── mymalloc.c
│   └── mymalloc.h
└── net
    └── spdk_server.c
```

## Makefile Targets

- `make` - Build the project
- `make clean` - Clean build artifacts

## License

MIT License Copyright (c) [2025] [yesyesyeswe]