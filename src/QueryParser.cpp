#include "QueryParser.h"
#include <sstream>
#include <vector>
#include <algorithm>

QueryParser::QueryParser(Database& db) : db_(db) {}

std::string QueryParser::trim(const std::string& s) const {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

std::string QueryParser::execute(const std::string& line) {
    std::istringstream ss(trim(line));
    std::string cmd;
    ss >> cmd;

    // Uppercase the command
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "SET") {
        std::string key, value;
        ss >> key;
        std::getline(ss, value);
        value = trim(value);
        if (key.empty() || value.empty())
            return "ERR: SET requires key and value";
        db_.set(key, value);
        return "OK";

    } else if (cmd == "GET") {
        std::string key; ss >> key;
        if (key.empty()) return "ERR: GET requires a key";
        auto val = db_.get(key);
        return val ? *val : "(nil)";

    } else if (cmd == "DEL") {
        std::string key; ss >> key;
        if (key.empty()) return "ERR: DEL requires a key";
        return db_.del(key) ? "OK (deleted)" : "(nil) key not found";

    } else if (cmd == "SCAN") {
        std::string from, to;
        ss >> from >> to;
        if (from.empty() || to.empty())
            return "ERR: SCAN requires from and to keys";
        auto results = db_.scan(from, to);
        if (results.empty()) return "(empty)";
        std::ostringstream out;
        for (auto& [k, v] : results)
            out << k << " -> " << v << "\n";
        return out.str();

    } else if (cmd == "FLUSH") {
        db_.flush();
        return "OK (flushed to disk)";

    } else if (cmd == "STATS") {
        return db_.stats();

    } else if (cmd == "EXIT" || cmd == "QUIT") {
        return "QUIT";

    } else if (cmd.empty()) {
        return "";
    } else {
        return "ERR: Unknown command '" + cmd +
               "'. Commands: SET GET DEL SCAN STATS FLUSH EXIT";
    }
}
