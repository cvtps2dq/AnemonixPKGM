//
// Created by cv2 on 2/24/25.
//

#include "Database.h"

bool Database::packageExists(const std::string& packageName) {
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        exit(EXIT_FAILURE);
    }
    sqlite3_stmt* stmt;
    const char* sql = "SELECT COUNT(*) FROM packages WHERE name = ?";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << RED << "[ERROR] Failed to prepare packageExists query: " << sqlite3_errmsg(db) << RESET << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, packageName.c_str(), -1, SQLITE_STATIC);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return exists;
}