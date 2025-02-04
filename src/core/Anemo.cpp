//
// Created by cv2 on 04.02.2025.
//

#include "Anemo.h"

#include <colors.h>
#include <iostream>
#include <sqlite3.h>
#include <unordered_set>
#include <yaml-cpp/yaml.h>
#include <__filesystem/operations.h>
#include <__filesystem/recursive_directory_iterator.h>

#include "config.h"
#include "defines.h"

bool Anemo::install(const std::filesystem::path &package_root) {
    try {
        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        const auto name = config["name"].as<std::string>();
        const auto version = config["version"].as<std::string>();
        const auto arch = config["arch"].as<std::string>();

        std::cout << "🚀 Installing Package: " << GREEN << name << RESET << " v" << version << " [" << arch << "]\n";

        // Step 1: Check Dependencies
        std::vector<std::string> missing_deps;
        if (config["deps"]) {
            sqlite3* db;
            if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
                throw std::runtime_error("Failed to open database");
            }

            for (const auto& dep : config["deps"]) {
                std::string dep_name = dep.as<std::string>();
                sqlite3_stmt* stmt;
                const char* check_sql = "SELECT name FROM packages WHERE name = ?;";
                if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, nullptr) != SQLITE_OK) {
                    throw std::runtime_error(sqlite3_errmsg(db));
                }

                sqlite3_bind_text(stmt, 1, dep_name.c_str(), -1, SQLITE_STATIC);
                if (sqlite3_step(stmt) != SQLITE_ROW) {
                    missing_deps.push_back(dep_name);
                }

                sqlite3_finalize(stmt);
            }
            sqlite3_close(db);
        }

        // If there are missing dependencies, notify the user and exit.
        if (!missing_deps.empty()) {
            std::cout << RED << "\n🚨 Installation Failed: Missing Dependencies 🚨\n" << RESET;
            std::cout << "----------------------------------------\n";
            for (const auto& dep : missing_deps) {
                std::cout << "❌ " << YELLOW << dep << RESET << " is not installed.\n";
            }
            std::cout << "----------------------------------------\n";
            std::cout << "🔗 Please install the missing dependencies and try again.\n\n";
            return false;
        }

        std::cout << GREEN << "✅ All dependencies satisfied. Proceeding with installation.\n" << RESET;

        // Step 2: Confirm Installation
        std::cout << "Do you want to install this package? (y/n): ";
        char choice;
        std::cin >> choice;
        if (choice == 'n') {
            std::cerr << "❌ User aborted package installation\n";
            exit(EXIT_FAILURE);
        }

        // Step 3: Run Build Script
        const std::string command = "cd '" + package_root.string() + "' && bash build.anemonix";
        if (system(command.c_str()) != 0) {
            throw std::runtime_error("Build script failed");
        }

        // Step 4: Register Package in Database
        sqlite3* db;
        if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database");
        }

        sqlite3_stmt* stmt;
        if (const auto sql = "INSERT INTO packages (name, version, arch) VALUES (?, ?, ?);";
            sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, version.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, arch.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        sqlite3_finalize(stmt);

        // Step 5: Track Installed Files
        std::filesystem::path package_data_root = package_root / "package";
        if (std::filesystem::exists(package_data_root) && std::filesystem::is_directory(package_data_root)) {
            const char* file_insert_sql = "INSERT INTO files (package_name, file_path) VALUES (?, ?);";
            for (const auto& file : std::filesystem::recursive_directory_iterator(package_data_root)) {
                if (file.is_regular_file()) {
                    std::string relative_path = file.path().lexically_relative(package_data_root).string();
                    std::string installed_path = "/" + relative_path;

                    sqlite3_stmt* file_stmt;
                    if (sqlite3_prepare_v2(db, file_insert_sql, -1, &file_stmt, nullptr) != SQLITE_OK) {
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                    sqlite3_bind_text(file_stmt, 1, name.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_text(file_stmt, 2, installed_path.c_str(), -1, SQLITE_STATIC);
                    if (sqlite3_step(file_stmt) != SQLITE_DONE) {
                        throw std::runtime_error(sqlite3_errmsg(db));
                    }
                    sqlite3_finalize(file_stmt);
                }
            }
        }

        sqlite3_close(db);
        std::cout << DSTRING << GREEN << "✅ Successfully installed " << name << " v" << version << "!\n" << RESET;

        std::cout << DSTRING << CYAN << "Removing build files...\n" << RESET;
        remove_all(package_root);
        std::cout << DSTRING << GREEN << "✔️ Done.\n" << RESET;
        return true;

    } catch (const std::exception& e) {
        std::cerr << RED << "❌ Installation failed: " << e.what() << " :(\n" << RESET;
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




