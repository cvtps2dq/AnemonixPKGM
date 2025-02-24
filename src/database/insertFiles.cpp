//
// Created by cv2 on 2/24/25.
//
#include "Database.h"

bool Database::insertFiles(const std::string& packageName, const std::vector<std::string>& files) {
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }

    const char* sql = R"(
        INSERT INTO files (package_name, file_path) VALUES (?, ?);
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQLite Error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    // Begin transaction for batch insert
    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    for (const auto& file : files) {
        sqlite3_bind_text(stmt, 1, packageName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, file.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "SQLite Insert Error: " << sqlite3_errmsg(db) << std::endl;
            sqlite3_finalize(stmt);
            sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
            sqlite3_close(db);
            return false;
        }
        sqlite3_reset(stmt);
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}