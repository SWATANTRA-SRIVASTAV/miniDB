#pragma once
#include "Database.h"
#include <string>

// Supported commands:
//   SET key value
//   GET key
//   DEL key
//   SCAN from to
//   STATS
//   FLUSH
//   EXIT / QUIT

class QueryParser {
public:
    explicit QueryParser(Database& db);
    std::string execute(const std::string& line);

private:
    Database& db_;
    std::string trim(const std::string& s) const;
};
