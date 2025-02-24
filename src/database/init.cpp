//
// Created by cv2 on 2/24/25.
//
#include "Database.h"

bool Database::init() {
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }

    const char* sql = R"(
    CREATE TABLE IF NOT EXISTS packages (
        name TEXT PRIMARY KEY,
        version TEXT NOT NULL,
        author TEXT,
        description TEXT,
        arch TEXT NOT NULL,
        provides TEXT DEFAULT '',
        deps TEXT DEFAULT '',
        conflicts TEXT DEFAULT '',
        replaces TEXT DEFAULT '',
        protected INTEGER DEFAULT 0,
        broken INTEGER DEFAULT 0
    );

    CREATE TABLE IF NOT EXISTS files (
        package_name TEXT,
        file_path TEXT,
        FOREIGN KEY (package_name) REFERENCES packages(name) ON DELETE CASCADE
    );
    )";


    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error creating tables: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}