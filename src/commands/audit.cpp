//
// Created by cv2 on 04.02.2025.
//

#include <colors.h>
#include <iostream>
#include <sqlite3.h>
#include <sstream>

#if defined(__APPLE__) && defined(__MACH__)
    #include <fmt/format.h>
    #include <fmt/ranges.h>
#elif defined (__linux__)
    #include <fmt>
    #include <vector>
#endif


#include "Anemo.h"
#include "config.h"

bool Anemo::audit() {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    std::vector<std::pair<std::string, std::vector<std::string>>> broken_packages;

    // Get all broken packages
    sqlite3_stmt* stmt;
    if (auto select_sql = "SELECT name, missing_deps FROM broken_packages;"; sqlite3_prepare_v2(db, select_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error retrieving broken packages\n";
        sqlite3_close(db);
        return false;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string package_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string missing_deps_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        std::vector<std::string> missing_deps;
        if (!missing_deps_str.empty()) {
            std::stringstream ss(missing_deps_str);
            std::string dep;
            while (std::getline(ss, dep, ',')) {
                missing_deps.push_back(dep);
            }
        }

        broken_packages.emplace_back(package_name, missing_deps);
    }
    sqlite3_finalize(stmt);

    if (broken_packages.empty()) {
        std::cout << GREEN << "No broken packages detected.\n" << RESET;
        sqlite3_close(db);
        return true;
    }

    std::cout << RED << "\n ! Broken Packages Detected !\n" << RESET;
    std::cout << "----------------------------------------\n";
    for (const auto& [pkg, deps] : broken_packages) {
        std::cout << "-> " << YELLOW << pkg << RESET << " is missing dependencies: ";
        std::cout << fmt::format("{}", fmt::join(deps, ", ")) << "\n";
    }
    std::cout << "----------------------------------------\n";
    std::cout << "🔗 Install missing dependencies and re-run 'anemo audit' to update.\n\n";

    // Check if any broken packages have their dependencies resolved
    for (const auto& [pkg, deps] : broken_packages) {
        bool all_deps_satisfied = true;
        for (const auto& dep : deps) {
            sqlite3_stmt* dep_stmt;
            if (auto check_sql = "SELECT name FROM packages WHERE name = ?;"; sqlite3_prepare_v2(db, check_sql, -1, &dep_stmt, nullptr) != SQLITE_OK) {
                std::cerr << "Error checking dependencies\n";
                continue;
            }

            sqlite3_bind_text(dep_stmt, 1, dep.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(dep_stmt) != SQLITE_ROW) {
                all_deps_satisfied = false;
            }

            sqlite3_finalize(dep_stmt);
        }

        // If dependencies are resolved, remove from broken_packages
        if (all_deps_satisfied) {
            sqlite3_stmt* delete_stmt;
            if (auto delete_sql = "DELETE FROM broken_packages WHERE name = ?;"; sqlite3_prepare_v2(db, delete_sql, -1, &delete_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(delete_stmt, 1, pkg.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(delete_stmt);
                sqlite3_finalize(delete_stmt);
                std::cout << GREEN << "[OK] " << pkg << " is now fixed and removed from the broken list.\n" << RESET;
            }
        }
    }

    sqlite3_close(db);
    return true;
}
