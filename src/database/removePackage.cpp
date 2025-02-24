//
// Created by cv2 on 2/24/25.
//
#include "Database.h"

bool Database::removePackage(const std::string& packageName) {
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }

    {
        const char* sql = R"(
            DELETE FROM packages WHERE description = ?;
        )";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            std::string descPattern = "provided by: " + packageName;
            sqlite3_bind_text(stmt, 1, descPattern.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    // SQL statement to delete the package from the database
    const char* sql = R"(
        DELETE FROM packages WHERE name = ?;
    )";



    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQLite Error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return false;
    }

    // Bind the package name
    sqlite3_bind_text(stmt, 1, packageName.c_str(), -1, SQLITE_STATIC);

    // Execute the deletion
    bool success = (sqlite3_step(stmt) == SQLITE_DONE);

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return success;
}