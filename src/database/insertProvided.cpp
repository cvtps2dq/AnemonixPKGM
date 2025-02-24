//
// Created by cv2 on 2/24/25.
//
#include "Database.h"

void Database::insertProvided(const Package& parentPkg) {
    if (parentPkg.provides.empty()) return;

    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return;
    }

    const char* sql = R"(
        INSERT INTO packages (name, version, arch, author, description, protected, provides, depends, conflicts)
        VALUES (?, ?, ?, '', ?, ?, '', '', '');
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQLite Error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    std::vector<std::pair<std::string, std::string>> providedPkgs = Utilities::parseProvidedField(parentPkg.provides);

    for (const auto& [pkgName, pkgVersion] : providedPkgs) {
        sqlite3_bind_text(stmt, 1, pkgName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, pkgVersion.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, parentPkg.arch.c_str(), -1, SQLITE_STATIC);

        std::string desc = "provided by: " + parentPkg.name;
        sqlite3_bind_text(stmt, 4, desc.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, parentPkg.isProtected ? 1 : 0);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "SQLite Insert Error: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}