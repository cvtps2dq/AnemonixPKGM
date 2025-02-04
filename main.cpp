#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sys/stat.h>
#include <sqlite3.h>
#include <yaml-cpp/yaml.h>
#include <cstdlib>
#include <unistd.h>
#include <filesystem>
#include <archive.h>
#include <archive_entry.h>
#include <cstring>

const std::string DB_PATH = "/var/lib/anemonix/installed.db";
const std::vector<std::string> REQUIRED_DIRS = {
    "/var/lib/anemonix",
    "/var/lib/anemonix/cache",
    "/var/lib/anemonix/builds",
    "/var/lib/anemonix/packages"
};

void show_help() {
    std::cout << "Anemonix Package Manager\n";
    std::cout << "Usage: anemonix <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  init            Initialize Anemonix folders and database\n";
    std::cout << "  install <pkg>   Install a package\n";
    std::cout << "  remove <pkg>    Remove a package\n";
    std::cout << "  update          Update package lists\n";
    std::cout << "  build <pkg>     Build a package from source\n";
    std::cout << "  search <query>  Search for a package\n";
    std::cout << "  audit           Verify installed packages\n";
}

bool is_superuser() {
    return geteuid() == 0;
}

bool init_folders() {
    for (const auto& dir : REQUIRED_DIRS) {
        if (mkdir(dir.c_str(), 0755) != 0 && errno != EEXIST) {
            std::cerr << "Error: Failed to create directory " << dir << "\n";
            return false;
        }
    }
    return true;
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

bool validate_apkg(const std::string& package_path) {
    std::filesystem::path build_script = package_path + "/build.anemonix";
    std::filesystem::path metadata_file = package_path + "/anemonix.yaml";
    std::filesystem::path package_folder = package_path + "/package";

    if (!std::filesystem::exists(build_script)) {
        std::cerr << "Error: Missing build script." << std::endl;
        return false;
    }
    if (!std::filesystem::exists(metadata_file)) {
        std::cerr << "Error: Missing metadata file." << std::endl;
        return false;
    }
    if (!std::filesystem::exists(package_folder) || !std::filesystem::is_directory(package_folder)) {
        std::cerr << "Error: Missing or invalid package folder." << std::endl;
        return false;
    }
    return true;
}

bool untar_package(const std::string& package_path, const std::string& extract_to) {
    struct archive* a;
    struct archive_entry* entry;
    int r;

    a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    if ((r = archive_read_open_filename(a, package_path.c_str(), 10240))) {
        std::cerr << "Error: Failed to open package file: " << archive_error_string(a) << std::endl;
        archive_read_free(a);
        return false;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        std::string entry_path = extract_to + "/" + archive_entry_pathname(entry);
        archive_entry_set_pathname(entry, entry_path.c_str());
        r = archive_read_extract(a, entry, 0);
        if (r != ARCHIVE_OK) {
            std::cerr << "Error: Failed to extract file: " << archive_error_string(a) << std::endl;
            archive_read_free(a);
            return false;
        }
    }

    archive_read_free(a);
    return true;
}

bool install_package(const std::string& package_path) {
    if (!is_superuser()) {
        std::cerr << "Error: This command must be run as root!\n";
        return false;
    }

    if (!validate_apkg(package_path)) {
        std::cerr << "Error: Package integrity check failed." << std::endl;
        return false;
    }

    YAML::Node config = YAML::LoadFile(package_path + "/anemonix.yaml");
    if (!config["name"] || !config["version"] || !config["arch"]) {
        std::cerr << "Invalid package metadata\n";
        return false;
    }
    std::string name = config["name"].as<std::string>();
    std::string version = config["version"].as<std::string>();
    std::string arch = config["arch"].as<std::string>();

    std::string build_script = package_path + "/build.anemonix";
    std::string command = "bash " + build_script;
    if (system(command.c_str()) != 0) {
        std::cerr << "Error: Failed to execute build script\n";
        return false;
    }

    sqlite3* db;
    if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Error: Cannot open database\n";
        return false;
    }

    std::string sql = "INSERT INTO packages (name, version, arch) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Error preparing SQL statement: " << sqlite3_errmsg(db) << "\n";
        sqlite3_close(db);
        return false;
    }

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, version.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, arch.c_str(), -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Error inserting package: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    std::cout << "Package " << name << " installed successfully.\n";
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    std::string command = argv[1];

    if (command == "init" || command == "remove" || command == "install") {
        if (!is_superuser()) {
            std::cerr << "Error: This command must be run as root!\n";
            return 1;
        }
    }

    if (command == "init") {
        std::cout << "Initializing Anemonix...\n";
        if (!init_folders() || !init_db()) {
            std::cerr << "Failed to initialize Anemonix." << std::endl;
            return 1;
        }
        std::cout << "Anemonix initialized successfully.\n";
    } else if (command == "install") {
        if (argc < 3) {
            std::cerr << "Error: No package specified for installation.\n";
            return 1;
        }
        std::string package_path = argv[2];
        if (!untar_package(package_path, "/var/lib/anemonix/packages")) {
            return 1;
        }
        if (!install_package("/var/lib/anemonix/packages")) {
            return 1;
        }
    } else {
        std::cerr << "Unknown command: " << command << "\n";
        show_help();
        return 1;
    }

    return 0;
}