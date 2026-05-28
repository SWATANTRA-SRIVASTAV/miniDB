#include "WAL.h"
#include <iostream>
#include <cstring>
#include <stdexcept>

WAL::WAL(const std::string& wal_file) : filename_(wal_file) {
    file_.open(wal_file, std::ios::in | std::ios::out |
                         std::ios::binary | std::ios::app);
    if (!file_.is_open())
        file_.open(wal_file, std::ios::in | std::ios::out |
                             std::ios::binary | std::ios::trunc);
    if (!file_.is_open())
        throw std::runtime_error("Cannot open WAL file: " + wal_file);
}

WAL::~WAL() { if (file_.is_open()) file_.close(); }

uint32_t WAL::checksum(const std::string& key,
                        const std::string& value) const {
    uint32_t h = 2166136261u;
    for (char c : key)   { h ^= (uint8_t)c; h *= 16777619u; }
    for (char c : value) { h ^= (uint8_t)c; h *= 16777619u; }
    return h;
}

void WAL::append(WALOp op,
                 const std::string& key,
                 const std::string& value) {
    file_.seekp(0, std::ios::end);

    uint8_t  op_byte = static_cast<uint8_t>(op);
    uint32_t klen    = static_cast<uint32_t>(key.size());
    uint32_t vlen    = static_cast<uint32_t>(value.size());
    uint32_t csum    = checksum(key, value);

    file_.write(reinterpret_cast<char*>(&op_byte), 1);
    file_.write(reinterpret_cast<char*>(&klen),    4);
    file_.write(key.data(),                        klen);
    file_.write(reinterpret_cast<char*>(&vlen),    4);
    file_.write(value.data(),                      vlen);
    file_.write(reinterpret_cast<char*>(&csum),    4);
    file_.flush();
}

std::vector<WALEntry> WAL::recover() {
    std::vector<WALEntry> entries;
    file_.seekg(0, std::ios::beg);

    while (file_) {
        uint8_t  op_byte;
        uint32_t klen, vlen, stored_csum;

        if (!file_.read(reinterpret_cast<char*>(&op_byte), 1)) break;
        if (!file_.read(reinterpret_cast<char*>(&klen),    4)) break;

        std::string key(klen, '\0');
        if (!file_.read(key.data(), klen)) break;

        if (!file_.read(reinterpret_cast<char*>(&vlen), 4)) break;

        std::string value(vlen, '\0');
        if (!file_.read(value.data(), vlen)) break;

        if (!file_.read(reinterpret_cast<char*>(&stored_csum), 4)) break;

        if (stored_csum != checksum(key, value)) {
            std::cerr << "[WAL] Corrupted entry skipped\n";
            continue;
        }
        entries.push_back({static_cast<WALOp>(op_byte), key, value});
    }
    file_.clear();
    return entries;
}

void WAL::checkpoint() {
    file_.close();
    std::ofstream trunc(filename_, std::ios::binary | std::ios::trunc);
    trunc.close();
    file_.open(filename_, std::ios::in | std::ios::out |
                          std::ios::binary | std::ios::app);
}

void WAL::sync() { file_.flush(); }
