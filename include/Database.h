#pragma once
#include "DiskManager.h"
#include "BTree.h"
#include "WAL.h"
#include <string>
#include <optional>
#include <vector>
#include <mutex>
#include <shared_mutex>

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    // Core operations — thread-safe
    void        set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    bool        del(const std::string& key);
    std::vector<std::pair<std::string,std::string>>
                scan(const std::string& from, const std::string& to);

    void        flush();   // persist everything to disk
    std::string stats();   // page count, key count, etc.

private:
    DiskManager       disk_;
    WAL               wal_;
    BTree             btree_;
    mutable std::shared_mutex rwlock_;   // readers-writer lock

    void recoverFromWAL();
};
