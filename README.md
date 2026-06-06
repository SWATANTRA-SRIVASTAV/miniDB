# minidb

A key-value storage engine written in C++17, built to understand how databases work below the abstraction layer.

The project started as an exercise in page-based file I/O and grew into a working B-Tree implementation with a write-ahead log. It has real limitations (documented below) that came from actually building it rather than reading about it.

## Architecture
┌─────────────────────────────────────────┐
│  QueryParser  (CLI: SET GET DEL SCAN)   │
├─────────────────────────────────────────┤
│  Database     (shared_mutex R/W lock)   │
├──────────────┬──────────────────────────┤
│  BTree       │  WAL                     │
│  (disk index)│  (crash recovery)        │
├──────────────┴──────────────────────────┤
│  DiskManager  (4KB page I/O)            │
└─────────────────────────────────────────┘
**DiskManager** reads and writes fixed-size 4KB pages to a binary file. Page size matches the OS virtual memory page, which means a page read is a single I/O operation with no partial-page penalty.

**BTree** is an on-disk B-Tree with order 8. Each node is one page. Leaf nodes store key-value pairs; internal nodes store separator keys and child page IDs. Leaf nodes are linked via right-sibling pointers, which makes range scans a sequential walk rather than repeated tree traversals.

**WAL** (Write-Ahead Log) appends every SET and DEL to a sequential log file before touching the B-Tree. On startup, if the log is non-empty, it replays all entries before accepting new writes. Entries use FNV-1a checksums — corrupted entries are skipped rather than crashing.

**Database** wraps BTree and WAL behind a `std::shared_mutex`. Multiple readers hold shared locks concurrently; writes take an exclusive lock.

## Known limitations

**Deletion does not rebalance.** After removing a key, the leaf is written back as-is even if it's below minimum occupancy. The tree stays correct — all remaining keys are still findable — but space is not reclaimed. This is the same approach SQLite uses with its freelist. Full borrow-and-merge rebalancing is the next planned feature; a previous attempt caused parent separator keys to become inconsistent on certain merge paths, which produced a segfault. The fix requires careful separator surgery that I haven't got right yet.

**No buffer pool.** Every read goes to disk. A production engine would cache hot pages in memory. The benchmark numbers reflect cold reads.

## Build

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
./minidb [path]          # default path: minidb_data
```
minidb> SET user:1 alice
minidb> GET user:1
alice
minidb> SCAN user:1 user:9
minidb> DEL user:1
minidb> STATS
minidb> FLUSH
minidb> EXIT
## Run tests

```bash
cd build && ctest --output-on-failure
```

9 tests covering: SET/GET, missing keys, overwrite, delete, WAL recovery across restarts, concurrent writes from 8 threads, and stats output.

## Benchmark

```bash
cd build && ./benchmark
```

On a MacBook Pro M-series (no buffer pool, cold reads):
WRITE  10,000 keys:  ~25,000 ops/sec
READ   10,000 keys:  ~56,000 ops/sec
DELETE  5,000 keys:  ~38,000 ops/sec
These numbers drop significantly under concurrent load because every write holds an exclusive lock across the full B-Tree. Finer-grained locking (per-page or per-level) is not implemented.

## What I learned building this

The hardest part was not the B-Tree splits — those follow a clear algorithm. The hard part was page serialization: deciding exactly which bytes go at which offset, making sure deserialization inverts it perfectly, and debugging cases where a node read back differently than it was written. The sibling pointer bug (SCAN returning one leaf's results because of an unconditional `break`) took longer to find than the split logic took to write.
