# MiniDB — Key-Value Store in C++

A database engine built from scratch in C++17, demonstrating core systems programming and data structures concepts.

## Architecture
MiniDB
├── DiskManager   → page-based file I/O (4KB pages)
├── WAL           → write-ahead log for crash recovery
├── BTree         → O(log n) B-Tree index on disk
├── Database      → thread-safe API (shared_mutex)
└── QueryParser   → CLI command interpreter

## Features
- **B-Tree index** — O(log n) reads and writes on disk
- **WAL** — write-ahead logging for crash durability
- **Thread-safe** — readers-writer lock for concurrent access
- **Persistent** — all data survives restarts
- **CLI shell** — interactive query interface
- **Benchmarks** — ops/sec measurement for SET/GET/DEL

## Commands
SET key value   → store a key-value pair
GET key         → retrieve value (nil if missing)
DEL key         → delete a key
SCAN from to    → range scan between two keys
STATS           → page count, DB size, B-Tree root
FLUSH           → force flush to disk
EXIT            → quit
## Build & Run
```bash
mkdir build && cd build
cmake ..
make
./minidb                    # interactive CLI
./benchmark                 # performance numbers
ctest --output-on-failure   # run all tests
```

## Skills Demonstrated
C++17 · B-Tree · WAL · File I/O · pthreads · shared_mutex · CMake · Google Test · Systems Design · OOP
