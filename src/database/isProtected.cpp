//
// Created by cv2 on 2/24/25.
//

#include "Database.h"

bool Database::isProtected(const std::string& packageName) {
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }

    const char* sql = R"(
        SELECT protected FROM packages WHERE name = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQLite Error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, packageName.c_str(), -1, SQLITE_STATIC);

    bool is_protected = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        is_protected = sqlite3_column_int(stmt, 0) == 1;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return is_protected;
}