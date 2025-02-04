//
// Created by cv2 on 04.02.2025.
//

#include "Database.h"

#include <iostream>
#include <sqlite3.h>
#include "config.h"

bool Database::init() {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }

    const auto sql = R"(
        CREATE TABLE IF NOT EXISTS packages (
            name TEXT PRIMARY KEY,
            version TEXT,
            arch TEXT
        );

        CREATE TABLE IF NOT EXISTS files (
            package_name TEXT NOT NULL,
            file_path TEXT NOT NULL,
            FOREIGN KEY(package_name) REFERENCES packages(name) ON DELETE CASCADE
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
