#include "Database.h"
#include "QueryParser.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string path = (argc > 1) ? argv[1] : "minidb_data";

    std::cout << "=============================================\n";
    std::cout << "  MiniDB — Key-Value Store (C++17)\n";
    std::cout << "  DB path: " << path << "\n";
    std::cout << "  Commands: SET GET DEL SCAN STATS FLUSH EXIT\n";
    std::cout << "=============================================\n\n";

    try {
        Database    db(path);
        QueryParser parser(db);

        std::string line;
        while (true) {
            std::cout << "minidb> ";
            if (!std::getline(std::cin, line)) break;
            if (line.empty()) continue;

            std::string result = parser.execute(line);
            if (result == "QUIT") {
                std::cout << "Bye!\n";
                break;
            }
            if (!result.empty())
                std::cout << result << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
