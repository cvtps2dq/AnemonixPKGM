//
// Created by cv2 on 04.02.2025.
//

#include "Database.h"
#include <iostream>
#include <sqlite3.h>
#include "config.h"
#include <vector>

bool Database::init() {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }

    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS packages (
            name TEXT PRIMARY KEY,
            version TEXT,
            arch TEXT,
            deps TEXT DEFAULT ''
        );

        CREATE TABLE IF NOT EXISTS files (
            package_name TEXT,
            file_path TEXT,
            FOREIGN KEY (package_name) REFERENCES packages(name) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS broken_packages (
            name TEXT PRIMARY KEY,
            missing_deps TEXT DEFAULT '',
            FOREIGN KEY (name) REFERENCES packages(name) ON DELETE CASCADE
        );
    )";

    char* errMsg;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error creating tables: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

std::vector<std::string> Database::checkDependencies(const std::vector<std::string> deps, const std::string& name){
    
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    std::vector<std::string> missing_deps;
    for (const auto& dep : deps) {
        sqlite3_stmt* dep_stmt;
        if (auto dep_check_sql = "SELECT name FROM packages WHERE name = ?;"; sqlite3_prepare_v2(db, dep_check_sql, -1, &dep_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(dep_stmt, 1, dep.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(dep_stmt) != SQLITE_ROW) {
                missing_deps.push_back(dep);
            }
            sqlite3_finalize(dep_stmt);
        }
    }

    sqlite3_close(db);
    return missing_deps;
}
