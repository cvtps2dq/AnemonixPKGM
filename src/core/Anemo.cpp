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

#include "config.h"
#include "defines.h"

bool Anemo::install(const std::filesystem::path &package_root) {
    try {
        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        const auto name = config["name"].as<std::string>();
        const auto version = config["version"].as<std::string>();
        const auto arch = config["arch"].as<std::string>();

        std::cout <<"name: " << name << " version: " << version << " arch: " << arch << "\n";
        std::cout << "install? y/n ";
        char choice;
        std::cin >> choice;
        if (choice == 'n') {
            std::cerr << "user aborted package installation\n";
            exit(EXIT_FAILURE);
        }
        if (choice == 'y') {
            const std::string command = "cd '" + package_root.string() + "' && bash build.anemonix";
            if (system(command.c_str()) != 0) {
                throw std::runtime_error("Build script failed");
            }

            // Database operations
            std::cout << DSTRING << CYAN << "opening database..." << std::endl << RESET;
            sqlite3* db;
            if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
                throw std::runtime_error("Failed to open database");
            }

            std::cout << DSTRING << CYAN << "querying database package registration..." << std::endl << RESET;
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
            sqlite3_close(db);
            std::cout << DSTRING << CYAN << "finalizing database statement..." << std::endl << RESET;

            std::cout << DSTRING << GREEN << "Successfully installed " << name << " v" << version << "\n" << RESET;
            return true;
        }
        std::cerr << "Unknown choice.\n";
        exit(EXIT_FAILURE);
    } catch (const std::exception& e) {
        std::cerr << "installation failed: " << e.what() << " :(" << "\n";
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




