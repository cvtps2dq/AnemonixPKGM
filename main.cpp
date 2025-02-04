#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sqlite3.h>
#include <yaml-cpp/yaml.h>
#include <cstdlib>

const std::string DB_PATH = "/var/lib/anemonix/installed.db";

void show_help() {
    std::cout << "Anemonix Package Manager\n";
    std::cout << "Usage: anemonix <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  install <pkg>   Install a package\n";
    std::cout << "  remove <pkg>    Remove a package\n";
    std::cout << "  update          Update package lists\n";
    std::cout << "  build <pkg>     Build a package from source\n";
    std::cout << "  search <query>  Search for a package\n";
    std::cout << "  audit           Verify installed packages\n";
}

bool init_db() {
    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }
    const char* sql = "CREATE TABLE IF NOT EXISTS packages (name TEXT PRIMARY KEY, version TEXT, arch TEXT);";
    char* errMsg;
    if (sqlite3_exec(db, sql, 0, 0, &errMsg) != SQLITE_OK) {
        std::cerr << "Error creating table: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }
    sqlite3_close(db);
    return true;
}

bool install_package(const std::string& package_path) {
    YAML::Node config = YAML::LoadFile(package_path + "/anemonix.yaml");
    if (!config["name"] || !config["version"] || !config["arch"]) {
        std::cerr << "Invalid package metadata\n";
        return false;
    }
    std::string name = config["name"].as<std::string>();
    std::string version = config["version"].as<std::string>();
    std::string arch = config["arch"].as<std::string>();

    std::string build_script = package_path + "/build.anemonix";
    if (std::ifstream(build_script)) {
        std::string command = "bash " + build_script;
        if (system(command.c_str()) != 0) {
            std::cerr << "Error: Failed to execute build script\n";
            return false;
        }
    } else {
        std::cerr << "Error: Build script not found\n";
        return false;
    }

    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }

    std::string sql = "INSERT INTO packages (name, version, arch) VALUES ('" + name + "', '" + version + "', '" + arch + "');";
    char* errMsg;
    if (sqlite3_exec(db, sql.c_str(), 0, 0, &errMsg) != SQLITE_OK) {
        std::cerr << "Error inserting package: " << errMsg << "\n";
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }
    sqlite3_close(db);

    std::cout << "Package " << name << " installed successfully.\n";
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    if (!init_db()) {
        std::cerr << "Failed to initialize database." << std::endl;
        return 1;
    }

    std::string command = argv[1];
    if (command == "install") {
        if (argc < 3) {
            std::cerr << "Error: No package specified for installation.\n";
            return 1;
        }
        std::string package_path = argv[2];
        if (!install_package(package_path)) {
            return 1;
        }
    } else if (command == "remove") {
        if (argc < 3) {
            std::cerr << "Error: No package specified for removal.\n";
            return 1;
        }
        std::string package = argv[2];
        std::cout << "Removing " << package << "...\n";
        // TODO: Implement removal logic
    } else if (command == "update") {
        std::cout << "Updating package lists...\n";
        // TODO: Implement update logic
    } else if (command == "build") {
        if (argc < 3) {
            std::cerr << "Error: No package specified for building.\n";
            return 1;
        }
        std::string package = argv[2];
        std::cout << "Building " << package << " from source...\n";
        // TODO: Implement build logic
    } else if (command == "search") {
        if (argc < 3) {
            std::cerr << "Error: No search query provided.\n";
            return 1;
        }
        std::string query = argv[2];
        std::cout << "Searching for " << query << "...\n";
        // TODO: Implement search logic
    } else if (command == "audit") {
        std::cout << "Auditing installed packages...\n";
        // TODO: Implement audit logic
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        show_help();
        return 1;
    }

    return 0;
}
