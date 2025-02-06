//
// Created by cv2 on 04.02.2025.
//

#include "Database.h"
#include <iostream>
#include <sqlite3.h>
#include <sstream>

#include "config.h"
#include <vector>
#include <colors.h>

bool Database::init() {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }

    const auto sql = R"(
        CREATE TABLE IF NOT EXISTS packages (
            name TEXT PRIMARY KEY,
            version TEXT,
            arch TEXT,
            deps TEXT DEFAULT '',
            description TEXT DEFAULT ''
        );

        CREATE TABLE IF NOT EXISTS files (
            package_name TEXT,
            file_path TEXT,
            FOREIGN KEY (package_name) REFERENCES packages(name) ON DELETE CASCADE
        );

        CREATE TABLE IF NOT EXISTS broken_packages (
            name TEXT PRIMARY KEY,
            missing_deps TEXT DEFAULT '',
            description TEXT DEFAULT '',
            FOREIGN KEY (name) REFERENCES packages(name) ON DELETE CASCADE
        );
    )";

    char* errMsg;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error creating tables: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

std::vector<std::string> Database::checkDependencies(const std::vector<std::string>& deps){
    
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    std::vector<std::string> missing_deps;
    for (const auto& dep : deps) {
        sqlite3_stmt* dep_stmt;
        if (const auto dep_check_sql = "SELECT name FROM packages WHERE name = ?;";
            sqlite3_prepare_v2(db, dep_check_sql, -1, &dep_stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(dep_stmt, 1, dep.c_str(), -1, SQLITE_STATIC);
            if (sqlite3_step(dep_stmt) != SQLITE_ROW) {
                missing_deps.push_back(dep);
            }
            sqlite3_finalize(dep_stmt);
        }
    }

    sqlite3_close(db);
    return missing_deps;
}

std::string Database::getPkgVersion(const std::string &name) {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    std::string installed_version;
    sqlite3_stmt* check_stmt;
    if (const auto check_sql = "SELECT version FROM packages WHERE name = ?;";
        sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(check_stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(check_stmt) == SQLITE_ROW) {
            installed_version = reinterpret_cast<const char*>(sqlite3_column_text(check_stmt, 0));
        }
        sqlite3_finalize(check_stmt);
    }
    sqlite3_close(db);
    return installed_version;
}

bool Database::insertPkg(const std::string &name, const std::string &version, const std::string &arch, const std::string &deps_str, const std::string &description) {

    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    sqlite3_stmt* stmt;
    if (const auto insert_sql = "INSERT INTO packages (name, version, arch, deps, description) VALUES (?, ?, ?, ?, ?);";
        sqlite3_prepare_v2(db, insert_sql, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, version.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, arch.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, deps_str.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, description.c_str(), -1, SQLITE_STATIC);
        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
    }
    sqlite3_close(db);
    return true;
}

bool Database::markAsBroken(const std::string &name, const std::string &deps_str) {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    sqlite3_stmt* broken_stmt;
    if (const auto insert_broken_sql = "INSERT OR REPLACE INTO broken_packages (name, missing_deps) VALUES (?, ?);";
        sqlite3_prepare_v2(db, insert_broken_sql, -1, &broken_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(broken_stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(broken_stmt, 2, deps_str.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(broken_stmt);
    } else {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    sqlite3_finalize(broken_stmt);
    sqlite3_close(db);
    return true;
}

std::vector<std::string> Database::checkPkgReliance(const std::string &name) {

    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
    // Check if this package is a dependency of another installed package
    sqlite3_stmt* stmt;
    if (const auto check_sql = "SELECT name FROM packages WHERE deps LIKE ?;"; sqlite3_prepare_v2(db, check_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    const std::string dep_pattern = "%"+name+"%";
    sqlite3_bind_text(stmt, 1, dep_pattern.c_str(), -1, SQLITE_STATIC);

    std::vector<std::string> dependent_packages;
    dependent_packages.reserve(1);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        dependent_packages.resize(dependent_packages.size() + 1);
        dependent_packages.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);

    return dependent_packages;

}

bool Database::writePkgFilesRecord(const std::string &name, const std::string &target_path) {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    sqlite3_stmt* file_stmt;
    if (const auto insert_file_sql = "INSERT INTO files (package_name, file_path) VALUES (?, ?);";
        sqlite3_prepare_v2(db, insert_file_sql, -1, &file_stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_text(file_stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(file_stmt, 2, target_path.c_str(), -1, SQLITE_STATIC);
        sqlite3_step(file_stmt);
        sqlite3_finalize(file_stmt);
    } else {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
     sqlite3_close(db);
    return true;
}

std::tuple<std::string, std::string> Database::fetchNameAndVersion(const std::string &name) {
    // Fetch package metadata
    std::string version, arch;

    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
    sqlite3_stmt* stmt;

    if (const auto select_metadata_sql = "SELECT version, arch FROM packages WHERE name = ?;";
        sqlite3_prepare_v2(db, select_metadata_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        arch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    } else {
        std::cerr << "Package '" << name << "' is not installed.\n";
        sqlite3_close(db);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return std::make_tuple(std::string(version), std::string(arch));

}

std::vector<std::string> Database::fetchFiles(const std::string &name) {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
    sqlite3_stmt* stmt;

    std::vector<std::string> files;
    if (const auto select_files_sql = "SELECT file_path FROM files WHERE package_name = ?;"; sqlite3_prepare_v2(db, select_files_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        files.emplace_back( AConf::BSTRAP_PATH + reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return files;
}

bool Database::markAffected(const std::vector<std::string>& dependent_packages) {

    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
    sqlite3_stmt* stmt;

    const auto create_broken_table_sql = "CREATE TABLE IF NOT EXISTS broken_packages (name TEXT PRIMARY KEY);";
    sqlite3_exec(db, create_broken_table_sql, nullptr, nullptr, nullptr);

    for (const auto& pkg : dependent_packages) {
        if (const auto insert_broken_sql = "INSERT INTO broken_packages (name) VALUES (?);";
            sqlite3_prepare_v2(db, insert_broken_sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, pkg.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }
    sqlite3_close(db);
    return true;
}

bool Database::removePkg(const std::string &name) {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }
    sqlite3_stmt* stmt;
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

    if (const auto delete_package_sql = "DELETE FROM broken_packages WHERE name = ?;"; sqlite3_prepare_v2(db, delete_package_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);


    sqlite3_close(db);

    return true;
}

std::vector<std::tuple<std::string, std::string, std::string>> Database::fetchAllPkgs() {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Failed to open database\n";
        throw std::runtime_error("Failed to open database");
    }

    std::vector<std::tuple<std::string, std::string, std::string>> packages;
    sqlite3_stmt* stmt;

    if (const auto query = "SELECT name, version, arch FROM packages;"; sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        throw std::runtime_error("Failed to prepare query");
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string arch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        packages.emplace_back(name, version, arch);
    }
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return packages;
}

std::vector<std::pair<std::string, std::vector<std::string>>> Database::fetchAllBroken() {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Failed to open database\n";
        throw std::runtime_error("Failed to open database");
    }
    std::vector<std::pair<std::string, std::vector<std::string>>> packages;

    // Get all broken packages
    sqlite3_stmt* stmt;
    if (auto select_sql = "SELECT name, missing_deps FROM broken_packages;"; sqlite3_prepare_v2(db, select_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error retrieving broken packages\n";
        sqlite3_close(db);
        throw std::runtime_error("Failed to prepare query");
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

        packages.emplace_back(package_name, missing_deps);
    }
    sqlite3_finalize(stmt);
    return packages;
}

bool Database::auditPkgs(const std::vector<std::pair<std::string,
    std::vector<std::string>>>& broken_packages) {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Failed to open database\n";
        throw std::runtime_error("Failed to open database");
    }
    for (const auto& [pkg, deps] : broken_packages) {
        bool all_deps_satisfied = true;
        for (const auto& dep : deps) {
            sqlite3_stmt* dep_stmt;
            if (const auto check_sql = "SELECT name FROM packages WHERE name = ?;"; sqlite3_prepare_v2(db, check_sql, -1, &dep_stmt, nullptr) != SQLITE_OK) {
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
            if (const auto delete_sql = "DELETE FROM broken_packages WHERE name = ?;"; sqlite3_prepare_v2(db, delete_sql, -1, &delete_stmt, nullptr) == SQLITE_OK) {
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

// std::vector<std::string> Database::fetchProvidedPackages(const std::string &name) {
//     std::vector<std::string> provided_packages;
//     try {
//         sqlite3 *db;
//         sqlite3_stmt *stmt;
//         const std::string query = "SELECT name FROM packages WHERE description LIKE 'provided by: " + name + "'";
//
//         if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
//             std::cerr << "Failed to open database\n";
//             throw std::runtime_error("Failed to open database");
//         }
//
//         if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
//             sqlite3_close(db);
//             throw std::runtime_error("Failed to prepare query");
//         }
//
//         provided_packages.reserve(1);
//         while (sqlite3_step(stmt) == SQLITE_ROW) {
//             std::cout << "fetch prov: " << reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)) << std::endl;
//             provided_packages.emplace_back(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0)));
//         }
//
//         sqlite3_finalize(stmt);
//         sqlite3_close(db);
//     } catch (const std::exception &e) {
//         std::cerr << "Database error: " << e.what() << std::endl;
//     }
//
//     return provided_packages;
// }

std::vector<std::string> Database::fetchProvidedPackages(const std::string &name) {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    sqlite3_stmt* stmt;
    std::vector<std::string> provided_packages;

    const char* select_provides_sql = "SELECT name, version FROM packages WHERE description = ?;";
    if (sqlite3_prepare_v2(db, select_provides_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    std::string provided_by_text = "provided by: " + name;
    sqlite3_bind_text(stmt, 1, provided_by_text.c_str(), -1, SQLITE_STATIC);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string provided_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string provided_version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        provided_packages.emplace_back(provided_name.append("=") + provided_version);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return provided_packages;
}

std::string Database::fetchDescription(const std::string& name) {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    sqlite3_stmt* stmt;
    std::vector<std::string> provided_packages;

    const char* select_provides_sql = "SELECT description FROM packages WHERE name LIKE = ?;";
    if (sqlite3_prepare_v2(db, select_provides_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    std::string provided_desc;
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        provided_desc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return provided_desc;
}