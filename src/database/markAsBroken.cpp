//
// Created by cv2 on 2/24/25.
//
#include "Database.h"

void Database::markPackageAsBroken(const std::string& packageName) {
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return;
    }

    const char* sql = "UPDATE packages SET broken = 1 WHERE name = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, packageName.c_str(), -1, SQLITE_STATIC);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}