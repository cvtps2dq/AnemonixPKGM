//
// Created by cv2 on 04.02.2025.
//

#include "Anemo.h"

#include <colors.h>
#include <iostream>
#include <sqlite3.h>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/operations.h>
#elif defined (__linux__)
    #include <filesystem>
    #include <vector>
#endif

#include "config.h"
#include <fmt/format.h>
#include <fmt/ranges.h>

int compareVersions(const std::string& v1, const std::string& v2) {
    std::vector<int> nums1, nums2;
    std::stringstream ss1(v1), ss2(v2);
    std::string temp;

    while (std::getline(ss1, temp, '.')) nums1.push_back(std::stoi(temp));
    while (std::getline(ss2, temp, '.')) nums2.push_back(std::stoi(temp));

    while (nums1.size() < nums2.size()) nums1.push_back(0);
    while (nums2.size() < nums1.size()) nums2.push_back(0);

    for (size_t i = 0; i < nums1.size(); i++) {
        if (nums1[i] < nums2[i]) return -1;
        if (nums1[i] > nums2[i]) return 1;
    }
    return 0;
}

bool Anemo::install(const std::filesystem::path &package_root, bool force, bool reinstall) {
    try {
        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        const auto name = config["name"].as<std::string>();
        const auto version = config["version"].as<std::string>();
        const auto arch = config["arch"].as<std::string>();

        std::vector<std::string> dependencies;
        if (config["deps"]) {
            for (const auto& dep : config["deps"]) {
                dependencies.push_back(dep.as<std::string>());
            }
        }
        std::string deps_str = dependencies.empty() ? "" : fmt::format("{}", fmt::join(dependencies, ","));

        sqlite3* db;
        if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database");
        }

        // Check if package is already installed
        std::string installed_version;
        sqlite3_stmt* check_stmt;
        if (auto check_sql = "SELECT version FROM packages WHERE name = ?;"; sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(check_stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(check_stmt) == SQLITE_ROW) {
                installed_version = reinterpret_cast<const char*>(sqlite3_column_text(check_stmt, 0));
            }
            sqlite3_finalize(check_stmt);
        }

        // Handle version comparison if already installed
        if (!installed_version.empty()) {
            if (int cmp = compareVersions(version, installed_version); cmp > 0) {
                std::cout << YELLOW << "An update is available: " << installed_version << " -> " << version << RESET << "\n";
                std::cout << "Do you want to update? (y/n): ";
            } else if (cmp < 0) {
                std::cout << RED << "Warning: You are about to downgrade from " << installed_version << " to " << version << RESET << "\n";
                std::cout << "Do you want to proceed? (y/n): ";
            } else if (reinstall) {
                std::cout << YELLOW << "Reinstalling package: " << name << RESET << "\n";
                std::cout << "Are you sure? This will erase the current installation. (y/n): ";
            } else {
                std::cout << GREEN << "Package " << name << " is already installed and up-to-date!\n" << RESET;
                sqlite3_close(db);
                remove_all(package_root);
                return true;
            }

            char choice;
            std::cin >> choice;
            if (choice != 'y') {
                std::cout << "Installation aborted.\n";
                sqlite3_close(db);
                remove_all(package_root);
                return false;
            }

            // Remove old package before upgrade/downgrade/reinstall
            remove(name, force, true);
        }

        // Check for missing dependencies
        std::vector<std::string> missing_deps;
        for (const auto& dep : dependencies) {
            sqlite3_stmt* dep_stmt;
            if (auto dep_check_sql = "SELECT name FROM packages WHERE name = ?;"; sqlite3_prepare_v2(db, dep_check_sql, -1, &dep_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(dep_stmt, 1, dep.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(dep_stmt) != SQLITE_ROW) {
                    missing_deps.push_back(dep);
                }
                sqlite3_finalize(dep_stmt);
            }
        }

        if (!force && !missing_deps.empty()) {
            std::cout << RED << "\n! Installation Failed: Missing Dependencies !\n" << RESET;
            for (const auto& dep : missing_deps) {
                std::cout << "❌ " << YELLOW << dep << RESET << " is not installed.\n";
            }
            std::cout << "Use --force to install anyway.\n";
            sqlite3_close(db);
            return false;
        }
        if (!missing_deps.empty()) {
            std::cout << YELLOW << "Warning: Proceeding with installation despite missing dependencies.\n" << RESET;
        }

        // Confirm installation
        std::cout << "Do you want to install this package? (y/n): ";
        char choice;
        std::cin >> choice;
        if (choice == 'n') {
            std::cerr << "User aborted package installation\n";
            sqlite3_close(db);
            remove_all(package_root);
            return false;
        }

        // Run build script
        const std::string command = "cd '" + package_root.string() + "' && bash build.anemonix";
        if (system(command.c_str()) != 0) {
            throw std::runtime_error("Build script failed");
        }

        // Insert package into DB
        sqlite3_stmt* stmt;
        if (auto insert_sql = "INSERT INTO packages (name, version, arch, deps) VALUES (?, ?, ?, ?);"; sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, version.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 3, arch.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 4, deps_str.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                throw std::runtime_error(sqlite3_errmsg(db));
            }
            sqlite3_finalize(stmt);
        }

        // Mark package as broken if dependencies are missing
        if (!missing_deps.empty()) {
            sqlite3_stmt* broken_stmt;
            if (auto insert_broken_sql = "INSERT OR REPLACE INTO broken_packages (name, missing_deps) VALUES (?, ?);"; sqlite3_prepare_v2(db, insert_broken_sql, -1, &broken_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(broken_stmt, 1, name.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(broken_stmt, 2, deps_str.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(broken_stmt);
                sqlite3_finalize(broken_stmt);
            }
        }

        sqlite3_close(db);

        std::cout << GREEN << "✅ Successfully installed " << name << " v" << version << "\n" << RESET;
        remove_all(package_root);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Installation failed: " << e.what() << "\n";
        return false;
    }
}


bool Anemo::validate(const std::filesystem::path& package_root) {
    const std::unordered_set<std::string> REQUIRED_FILES = {
        "build.anemonix",
        "anemonix.yaml",
        "package/"
    };

    for (const auto& item : REQUIRED_FILES) {
        if (std::filesystem::path path = package_root / item; !exists(path)) {
            std::cerr << "err: missing required package item: " << item << "\n";
            return false;
        }
    }
    return true;
}

void Anemo::showHelp() {
    std::cout << "anemo package manager\n";
    std::cout << "usage: anemo <command> [options]\n\n";
    std::cout << "commands:\n";
    std::cout << "  init            initialize anemo folders and database\n";
    std::cout << "  install <pkg>   install a package\n";
    std::cout << "  remove <pkg>    remove a package\n";
    std::cout << "  update          update package lists\n";
    std::cout << "  build <pkg>     build a package from source\n";
    std::cout << "  search <query>  search for a package\n";
    std::cout << "  audit           verify installed packages\n";
}




