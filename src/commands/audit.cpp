//
// Created by cv2 on 04.02.2025.
//

#include <colors.h>
#include <iostream>
#include <sqlite3.h>
#include <sstream>

#include <fmt/format.h>
#include <fmt/ranges.h>
#if defined (__linux__)
    #include <vector>
#endif

#include <Database.h>
#include "Anemo.h"
#include "config.h"

bool Anemo::audit() {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    std::vector<std::pair<std::string,
    std::vector<std::string>>> broken_packages = Database::fetchAllBroken();


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
    std::cout << "Install missing dependencies and re-run 'anemo audit' to update.\n\n";

    Database::auditPkgs(broken_packages);
    return false;
}
