//
// Created by cv2 on 2/24/25.
//

#include "Database.h"

std::vector<std::string> Database::getPackageFiles(const std::string& packageName) {
    std::vector<std::string> files;

    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return files;
    }

    const char* sql = R"(
        SELECT file_path FROM files WHERE package_name = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQLite Error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return files;
    }

    sqlite3_bind_text(stmt, 1, packageName.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        files.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return files;
}