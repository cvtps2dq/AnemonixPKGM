//
// Created by cv2 on 2/24/25.
//
#include "Database.h"

std::vector<std::pair<std::string, std::string>> parseProvidedField(const std::string& provides) {
    std::vector<std::pair<std::string, std::string>> result;
    std::stringstream ss(provides);
    std::string token;

    while (getline(ss, token, ',')) {
        size_t eqPos = token.find('=');
        if (eqPos != std::string::npos) {
            std::string name = token.substr(0, eqPos);
            std::string version = token.substr(eqPos + 1);
            result.emplace_back(name, version);
        } else {
            result.emplace_back(token, ""); // No version provided
        }
    }
    return result;
}

void Database::insertProvided(const Package& parentPkg) {
    if (parentPkg.provides.empty()) return;

    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return;
    }

    const char* sql = R"(
        INSERT INTO packages (name, version, arch, author, description, protected, provides, deps, conflicts)
        VALUES (?, ?, ?, '', ?, ?, '', '', '');
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQLite Error: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }

    std::string prov_str = parentPkg.provides.empty() ? "" :
    fmt::format("{}", fmt::join(parentPkg.provides, ","));

    std::vector<std::pair<std::string, std::string>> providedPkgs = parseProvidedField(prov_str);

    for (const auto& [pkgName, pkgVersion] : providedPkgs) {
        sqlite3_bind_text(stmt, 1, pkgName.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, pkgVersion.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, parentPkg.arch.c_str(), -1, SQLITE_STATIC);

        std::string desc = "provided by: " + parentPkg.name;
        sqlite3_bind_text(stmt, 4, desc.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, parentPkg.is_protected ? 1 : 0);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "SQLite Insert Error: " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
}
