//
// Created by cv2 on 04.02.2025.
//

#include <iostream>
#include <unistd.h>
#include <Utilities.h>
#include <yaml-cpp/yaml.h>
#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/operations.h>
#elif defined (__linux__)
    #include <filesystem>
    #include <vector>
#endif
#include "colors.h"
#include "Anemo.h"
#include "config.h"
#include "defines.h"
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <sqlite3.h>

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

bool installPkg(const std::filesystem::path &package_root, bool force, bool reinstall) {
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
            Anemo::remove(name, force, true);
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

        if (!force && !missing_deps.empty() && AConf::BSTRAP_PATH.empty()) {
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
        char choice;
        if (AConf::BSTRAP_PATH.empty()) {
            std::cout << "Do you want to install this package? (y/n): ";
            std::cin >> choice;
        } else {
            choice = 'y';
        }

        if (choice == 'n') {
            std::cerr << "User aborted package installation\n";
            sqlite3_close(db);
            remove_all(package_root);
            return false;
        }

        // Run build script
        for (std::filesystem::path package_dir = package_root / "package";
            const auto& file : std::filesystem::recursive_directory_iterator(package_dir)) {
            std::filesystem::path target_path = "/" / file.path().lexically_relative(package_dir);
            if (is_directory(file)) {
                std::filesystem::create_directories( AConf::BSTRAP_PATH + target_path.c_str());
            } else {
                // Insert copied file path into database
                sqlite3_stmt* file_stmt;
                if (auto insert_file_sql = "INSERT INTO files (package_name, file_path) VALUES (?, ?);"; sqlite3_prepare_v2(db, insert_file_sql, -1, &file_stmt, nullptr) == SQLITE_OK) {
                    sqlite3_bind_text(file_stmt, 1, name.c_str(), -1, SQLITE_STATIC);
                    sqlite3_bind_text(file_stmt, 2, target_path.c_str(), -1, SQLITE_STATIC);
                    sqlite3_step(file_stmt);
                    sqlite3_finalize(file_stmt);
                }
                copy(file, AConf::BSTRAP_PATH + target_path.c_str(), std::filesystem::copy_options::overwrite_existing);
            }
        }

        if (std::filesystem::path install_script = package_root / "install.anemonix"; std::filesystem::exists(install_script)) {
            if (system(install_script.string().c_str()) != 0) {
                throw std::runtime_error("install.anemonix script failed");
            }
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

bool Anemo::install(int argc, char* argv[], bool force, bool reinstall) {
    if (!Utilities::isSu()) {
            std::cerr << RED << "err: requires root privileges :( \n" << RESET;
            return false;
        }

        if (argc < 3) {
            std::cerr << RED << "err: missing package file :( \n" << RESET;
            return false;
        }

        // Create temporary extraction directory

        std::filesystem::path temp_dir = AConf::ANEMO_ROOT + "/builds/tmp_" + std::to_string(getpid());
        if (!create_directory(temp_dir)) {
            std::cerr << RED << "err: failed to create temp directory :( \n" << RESET;
            return false;
        }

        // Step 1: Untar package
        if (!Utilities::untarPKG(argv[2], temp_dir.string())) {
            remove_all(temp_dir);
            return false;
        }

        // Step 2: Find package root
        std::filesystem::path package_root = Utilities::findPKGRoot(temp_dir);
        if (package_root.empty()) {
            remove_all(temp_dir);
            return false;
        }

        // Step 3: Validate package
        if (!validate(package_root)) {
            remove_all(temp_dir);
            return false;
        }

        // Step 4: Read metadata and create target directory

        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        auto name = config["name"].as<std::string>();
        auto version = config["version"].as<std::string>();

        std::filesystem::path target_dir = AConf::ANEMO_ROOT + "/packages/" + name + "-" + version;

        // Move package to final location
        std::filesystem::rename(package_root, target_dir);
        remove_all(temp_dir);

        // Step 5: Install
        if (!installPkg(target_dir, force, reinstall)) {
            std::cout << DSTRING << RED << "install failed :(" << std::endl << RESET;
            std::cout << DSTRING << RED << "removing package dir: " << target_dir << std::endl << RESET;
            remove_all(target_dir);
            return false;
        }
    return true;
}
