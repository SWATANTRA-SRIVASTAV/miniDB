#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>

enum class WALOp : uint8_t { SET = 1, DEL = 2 };

struct WALEntry {
    WALOp        op;
    std::string  key;
    std::string  value;   // empty for DEL
};

// WAL file format (sequential records):
//  [op:1][key_len:4][key:key_len][val_len:4][val:val_len][checksum:4]
class WAL {
public:
    explicit WAL(const std::string& wal_file);
    ~WAL();

    void append(WALOp op, const std::string& key, const std::string& value = "");
    std::vector<WALEntry> recover();   // read all entries on startup
    void checkpoint();                 // truncate after successful flush
    void sync();                       // fsync to disk

private:
    std::fstream  file_;
    std::string   filename_;

    uint32_t checksum(const std::string& key, const std::string& value) const;
};
