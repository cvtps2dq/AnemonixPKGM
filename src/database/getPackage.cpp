//
// Created by cv2 on 2/24/25.
//

#include <Utilities.h>

#include "Database.h"

std::optional<Package> Database::getPackage(const std::string& packageName) {
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        exit(EXIT_FAILURE);
    }
    const char* sql = R"(
        SELECT name, version, author, description, arch, provides, deps, conflicts, replaces, protected
        FROM packages
        WHERE name = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQLite Error: " << sqlite3_errmsg(db) << std::endl;
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, packageName.c_str(), -1, SQLITE_STATIC);

    Package pkg("", "", "", "", "");  // Empty package object to populate

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        pkg.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        pkg.version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        pkg.author = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        pkg.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        pkg.arch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));

        // Convert stored CSV strings to vectors
        pkg.provides = Utilities::splitString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5)));
        pkg.deps = Utilities::splitString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        pkg.conflicts = Utilities::splitString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        pkg.replaces = Utilities::splitString(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));

        // Read 'protected' flag
        pkg.is_protected = sqlite3_column_int(stmt, 9) == 1;
    } else {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return std::nullopt;  // Package not found
    }

    sqlite3_finalize(stmt);
    return pkg;
}