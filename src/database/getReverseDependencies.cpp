//
// Created by cv2 on 2/24/25.
//

#include <Utilities.h>

#include "Database.h"

std::vector<std::string> parseDependencies(const std::string& depsString) {
    std::vector<std::string> deps;
    std::stringstream ss(depsString);
    std::string dep;

    while (std::getline(ss, dep, ',')) {
        dep.erase(0, dep.find_first_not_of(" \t"));  // Trim leading spaces
        dep.erase(dep.find_last_not_of(" \t") + 1);  // Trim trailing spaces

        if (!dep.empty()) {
            deps.push_back(dep);
        }
    }
    return deps;
}

std::vector<std::string> Database::getReverseDependencies(const std::string& packageName, const std::string& packageVersion) {
    std::vector<std::string> reverseDeps;

    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return reverseDeps;
    }

    const char* sql = R"(
        SELECT name, deps FROM packages;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQLite Error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return reverseDeps;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string depPackage = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string depsString = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        // Parse dependencies correctly
        std::vector<std::string> deps = parseDependencies(depsString);
        for (const std::string& dep : deps) {
            if (satisfiesDependency(dep, packageName, packageVersion)) {
                reverseDeps.push_back(depPackage);
                break;
            }
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return reverseDeps;
}