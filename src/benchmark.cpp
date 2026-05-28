#include "Database.h"
#include <iostream>
#include <chrono>
#include <string>
#include <cstdlib>

using Clock = std::chrono::high_resolution_clock;

int main() {
    const std::string path = "bench_data";
    const int N = 10000;

    std::cout << "=== MiniDB Benchmark ===\n";
    std::cout << "Operations: " << N << "\n\n";

    // Clean slate
    std::remove((path + ".db").c_str());
    std::remove((path + ".wal").c_str());

    Database db(path);

    // ── WRITE benchmark ──
    auto t0 = Clock::now();
    for (int i = 0; i < N; i++) {
        std::string key = "key:" + std::to_string(i);
        std::string val = "value:" + std::to_string(i * 42);
        db.set(key, val);
    }
    auto t1 = Clock::now();
    double write_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // ── READ benchmark ──
    int hits = 0;
    t0 = Clock::now();
    for (int i = 0; i < N; i++) {
        auto v = db.get("key:" + std::to_string(i));
        if (v) hits++;
    }
    t1 = Clock::now();
    double read_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    // ── DELETE benchmark ──
    t0 = Clock::now();
    for (int i = 0; i < N / 2; i++)
        db.del("key:" + std::to_string(i));
    t1 = Clock::now();
    double del_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();

    std::cout << "WRITE  " << N << " keys : "
              << write_ms << " ms  ("
              << (int)(N / (write_ms / 1000)) << " ops/sec)\n";
    std::cout << "READ   " << N << " keys : "
              << read_ms  << " ms  ("
              << (int)(N / (read_ms  / 1000)) << " ops/sec) — "
              << hits << " hits\n";
    std::cout << "DELETE " << N/2 << " keys : "
              << del_ms   << " ms  ("
              << (int)((N/2) / (del_ms / 1000)) << " ops/sec)\n";
    std::cout << "\n" << db.stats();

    // Cleanup
    std::remove((path + ".db").c_str());
    std::remove((path + ".wal").c_str());
    return 0;
}
