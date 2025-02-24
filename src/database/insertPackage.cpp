//
// Created by cv2 on 2/24/25.
//

#include "Database.h"

bool Database::insertPackage(const Package& pkg, bool broken) {
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }
    const char* sql = R"(
        INSERT INTO packages (name, version, author, description, arch, provides, deps, conflicts, replaces, protected, broken)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQLite Error: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    std::string deps_str = pkg.deps.empty() ? "" : fmt::format("{}", fmt::join(pkg.deps, ","));
    std::string con_str = pkg.conflicts.empty() ? "" : fmt::format("{}", fmt::join(pkg.conflicts, ","));
    std::string repl_str = pkg.replaces.empty() ? "" : fmt::format("{}", fmt::join(pkg.replaces, ","));
    std::string prov_str = pkg.provides.empty() ? "" : fmt::format("{}", fmt::join(pkg.provides, ","));

    // Bind values
    sqlite3_bind_text(stmt, 1, pkg.name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pkg.version.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, pkg.author.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, pkg.description.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 5, pkg.arch.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 6, prov_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, deps_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, con_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 9, repl_str.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 10, pkg.is_protected ? 1 : 0);  // Store as 1 (true) or 0 (false)
    sqlite3_bind_int(stmt, 11, broken);

    // Execute query
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "SQLite Insert Error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return true;
}