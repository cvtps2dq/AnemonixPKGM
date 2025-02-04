//
// Created by cv2 on 04.02.2025.
//

#include <iostream>
#include <sqlite3.h>
#include <__filesystem/operations.h>

#include "Anemo.h"
#include "config.h"

bool Anemo::remove(const std::string &name) {
    try {
        sqlite3* db;
        if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database");
        }

        // Fetch package metadata
        sqlite3_stmt* stmt;
        std::string version, arch;

        if (const auto select_metadata_sql = "SELECT version, arch FROM packages WHERE name = ?;"; sqlite3_prepare_v2(db, select_metadata_sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            arch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        } else {
            std::cerr << "Package '" << name << "' is not installed.\n";
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return false;
        }
        sqlite3_finalize(stmt);

        // Fetch file list
        std::vector<std::string> files;
        if (const auto select_files_sql = "SELECT file_path FROM files WHERE package_name = ?;"; sqlite3_prepare_v2(db, select_files_sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }

        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            files.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        sqlite3_finalize(stmt);

        if (files.empty()) {
            std::cerr << "No files recorded for package '" << name << "'.\n";
            sqlite3_close(db);
            return false;
        }

        // Print metadata & files in a clean format
        std::cout << "\n! Package Removal Confirmation !\n";
        std::cout << "------------------------------------\n";
        std::cout << "Name    : " << name << "\n";
        std::cout << "Version : " << version << "\n";
        std::cout << "Arch    : " << arch << "\n";
        std::cout << "------------------------------------\n";
        std::cout << "Installed Files:\n";
        for (const auto& file : files) {
            std::cout << "   -> " << file << "\n";
        }
        std::cout << "------------------------------------\n";
        std::cout << "Proceed with removal? (y/n): ";

        char choice;
        std::cin >> choice;
        if (choice == 'n') {
            std::cerr << "User aborted package removal.\n";
            sqlite3_close(db);
            return false;
        }
        if (choice != 'y') {
            std::cerr << "Invalid choice.\n";
            sqlite3_close(db);
            return false;
        }

        // Remove files
        for (const auto& file : files) {
            std::filesystem::remove(file);
        }

        // Remove database entries
        if (const auto delete_files_sql = "DELETE FROM files WHERE package_name = ?;"; sqlite3_prepare_v2(db, delete_files_sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (const auto delete_package_sql = "DELETE FROM packages WHERE name = ?;"; sqlite3_prepare_v2(db, delete_package_sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        sqlite3_close(db);

        std::cout << "[OK] Successfully removed " << name << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[FL] Removal failed: " << e.what() << " :(\n";
        return false;
    }
}