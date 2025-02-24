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
#include <unordered_set>

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
    std::string version, arch;
    sqlite3* db;

    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(db)));
    }

    sqlite3_stmt* stmt;
    const char* sql = "SELECT version, arch FROM packages WHERE name = ?;";

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::string errorMsg = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw std::runtime_error("SQL error: " + errorMsg);
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        arch = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    } else {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        std::cerr << "Package not found: " << name << std::endl;
        exit(EXIT_FAILURE);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return std::make_tuple(version, arch);
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
    sqlite3_stmt* stmt;

    // Query all installed packages
    std::unordered_set<std::string> installed_packages;
    if (sqlite3_prepare_v2(db, "SELECT name FROM packages;", -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            installed_packages.insert(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
    }
    sqlite3_finalize(stmt);

    // Get all broken packages
    if (sqlite3_prepare_v2(db, "SELECT name, missing_deps FROM broken_packages;", -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error retrieving broken packages\n";
        sqlite3_close(db);
        throw std::runtime_error("Failed to prepare query");
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string package_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string missing_deps_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        std::vector<std::string> missing_deps;
        std::stringstream ss(missing_deps_str);
        std::string dep;
        while (std::getline(ss, dep, ',')) {
            if (!installed_packages.contains(dep)) {
                missing_deps.push_back(dep);  // Keep only missing dependencies
            }
        }

        if (missing_deps.empty()) {
            // If all dependencies are satisfied, remove the package from the broken list
            sqlite3_stmt* update_stmt;
            if (sqlite3_prepare_v2(db, "DELETE FROM broken_packages WHERE name = ?;", -1, &update_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(update_stmt, 1, package_name.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(update_stmt);
            }
            sqlite3_finalize(update_stmt);
        } else {
            // Update the missing dependencies in the database
            sqlite3_stmt* update_stmt;
            std::string new_missing_deps;
            for (const auto& d : missing_deps) {
                if (!new_missing_deps.empty()) new_missing_deps += ",";
                new_missing_deps += d;
            }
            if (sqlite3_prepare_v2(db, "UPDATE broken_packages SET missing_deps = ? WHERE name = ?;", -1, &update_stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(update_stmt, 1, new_missing_deps.c_str(), -1, SQLITE_STATIC);
                sqlite3_bind_text(update_stmt, 2, package_name.c_str(), -1, SQLITE_STATIC);
                sqlite3_step(update_stmt);
            }
            sqlite3_finalize(update_stmt);

            packages.emplace_back(package_name, missing_deps);
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
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

std::vector<std::string> Database::fetchProvidedPackages(const std::string &name) {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    sqlite3_stmt* stmt;
    std::vector<std::string> provided_packages;

    if (const auto select_provides_sql = "SELECT name, version FROM packages WHERE description = ?;"; sqlite3_prepare_v2(db, select_provides_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }

    const std::string provided_by_text = "provided by: " + name;
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

    if (const auto select_provides_sql = "SELECT description FROM packages WHERE name LIKE ?;"; sqlite3_prepare_v2(db, select_provides_sql, -1, &stmt, nullptr) != SQLITE_OK) {
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


void Database::writePkgFilesBatch(const std::string& name, const std::vector<std::string>& vector) {
    constexpr char spin_chars[] = {'|', '/', '-', '\\'};
    int spin_index = 0;
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open database");
    }

    char* errMsg = nullptr;
    if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to start transaction: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return;
    }

    sqlite3_stmt* stmt;

    if (const auto sql = "INSERT INTO files (package_name, file_path) VALUES (?, ?);"; sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return;
    }
    int ix = 0;
    for (const auto& file : vector) {
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, file.c_str(), -1, SQLITE_STATIC);
        if (ix % 4 == 0) {
            std::cout << "\r[ " << spin_chars[spin_index] << " ] registering files";
            spin_index = (spin_index + 1) % 4;
        }

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::cerr << "Failed to insert file: " << file << " -> " << sqlite3_errmsg(db) << std::endl;
        }

        sqlite3_reset(stmt); // Reset statement for next iteration
        ix++;
    }

    sqlite3_finalize(stmt);

    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to commit transaction: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }

    sqlite3_close(db);
}

int Database::countPackages() {
    sqlite3* db;
    if (sqlite3_open(AConf::DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "[Error] Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        return -1;
    }

    const char* sql = "SELECT COUNT(*) FROM packages;";
    sqlite3_stmt* stmt;
    int package_count = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Error] Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        package_count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    //std::cout << package_count << " packages installed." << std::endl;
    return package_count;
}


