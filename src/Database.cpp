#include "Database.h"
#include <sstream>
#include <iostream>

Database::Database(const std::string& path)
    : disk_(path + ".db"),
      wal_(path  + ".wal"),
      btree_(disk_)
{
    recoverFromWAL();
}

Database::~Database() { flush(); }

void Database::recoverFromWAL() {
    auto entries = wal_.recover();
    if (entries.empty()) return;
    std::cerr << "[DB] Replaying " << entries.size()
              << " WAL entries...\n";
    for (auto& e : entries) {
        if (e.op == WALOp::SET) btree_.insert(e.key, e.value);
        else                    btree_.remove(e.key);
    }
    wal_.checkpoint();
    disk_.flush();
    std::cerr << "[DB] Recovery complete.\n";
}

void Database::set(const std::string& key, const std::string& value) {
    std::unique_lock lock(rwlock_);
    wal_.append(WALOp::SET, key, value);
    btree_.insert(key, value);
}

std::optional<std::string> Database::get(const std::string& key) {
    std::shared_lock lock(rwlock_);
    return btree_.search(key);
}

bool Database::del(const std::string& key) {
    std::unique_lock lock(rwlock_);
    wal_.append(WALOp::DEL, key);
    return btree_.remove(key);
}

std::vector<std::pair<std::string,std::string>>
Database::scan(const std::string& from, const std::string& to) {
    std::shared_lock lock(rwlock_);
    return btree_.scan(from, to);
}

void Database::flush() {
    std::unique_lock lock(rwlock_);
    disk_.flush();
    wal_.checkpoint();
}

std::string Database::stats() {
    std::shared_lock lock(rwlock_);
    std::ostringstream ss;
    ss << "Pages on disk : " << disk_.pageCount() << "\n";
    ss << "Page size     : " << PAGE_SIZE << " bytes\n";
    ss << "DB file size  : "
       << (disk_.pageCount() * PAGE_SIZE / 1024) << " KB\n";
    ss << "B-Tree root   : page " << btree_.rootPageId() << "\n";
    return ss.str();
}
