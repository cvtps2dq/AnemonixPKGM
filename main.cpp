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
#include <sstream>
#include <unordered_set>

const std::string DB_PATH = "/var/lib/anemonix/installed.db";
const std::vector<std::string> REQUIRED_DIRS = {
    "/var/lib/anemonix",
    "/var/lib/anemonix/cache",
    "/var/lib/anemonix/builds",
    "/var/lib/anemonix/packages"
};

// ... [keep previous helper functions: show_help(), is_superuser(), init_folders(), init_db()] ...

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

bool validate_apkg(const std::filesystem::path& package_root) {
    const std::unordered_set<std::string> REQUIRED_FILES = {
        "build.anemonix",
        "anemonix.yaml",
        "package/"
    };

    for (const auto& item : REQUIRED_FILES) {
        std::filesystem::path path = package_root / item;
        if (!std::filesystem::exists(path)) {
            std::cerr << "Error: Missing required package item: " << item << "\n";
            return false;
        }
    }
    return true;
}

bool untar_package(const std::string& package_path, const std::string& extract_to) {
    struct archive* a = archive_read_new();
    archive_read_support_format_all(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, package_path.c_str(), 10240) != ARCHIVE_OK) {
        std::cerr << "Error opening package: " << archive_error_string(a) << "\n";
        archive_read_free(a);
        return false;
    }

    struct archive_entry* entry;
    int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_FFLAGS;

    struct archive* ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, flags);
    archive_write_disk_set_standard_lookup(ext);

    while (true) {
        int r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) break;
        if (r != ARCHIVE_OK) {
            std::cerr << "Error reading header: " << archive_error_string(a) << "\n";
            archive_read_free(a);
            archive_write_free(ext);
            return false;
        }

        std::string full_path = extract_to + "/" + archive_entry_pathname(entry);
        archive_entry_set_pathname(entry, full_path.c_str());

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            std::cerr << "Error writing header: " << archive_error_string(ext) << "\n";
        } else if (archive_entry_size(entry) > 0) {
            const void* buff;
            size_t size;
            la_int64_t offset;
            while (ARCHIVE_OK == archive_read_data_block(a, &buff, &size, &offset)) {
                if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
                    std::cerr << "Error writing data: " << archive_error_string(ext) << "\n";
                    break;
                }
            }
        }
        archive_write_finish_entry(ext);
    }

    archive_read_free(a);
    archive_write_free(ext);
    return true;
}

std::filesystem::path find_package_root(const std::filesystem::path& temp_dir) {
    // Check if metadata exists directly in temp dir
    if (std::filesystem::exists(temp_dir / "anemonix.yaml")) {
        return temp_dir;
    }

    // Look for a single subdirectory containing metadata
    std::filesystem::path package_root;
    for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
        if (entry.is_directory() && std::filesystem::exists(entry.path() / "anemonix.yaml")) {
            if (!package_root.empty()) {
                std::cerr << "Error: Multiple package roots found\n";
                return {};
            }
            package_root = entry.path();
        }
    }

    if (package_root.empty()) {
        std::cerr << "Error: No valid package structure found\n";
    }
    return package_root;
}

bool install_package(const std::filesystem::path& package_root) {
    try {
        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        std::string name = config["name"].as<std::string>();
        std::string version = config["version"].as<std::string>();
        std::string arch = config["arch"].as<std::string>();

        // Execute build script
        std::string build_script = (package_root / "build.anemonix").string();
        if (system(("bash cd " + (package_root/ ".").string()).c_str()) && system(("bash " + build_script).c_str()) != 0) {
            throw std::runtime_error("Build script failed");
        }

        // Database operations
        sqlite3* db;
        if (sqlite3_open(DB_PATH.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Failed to open database");
        }

        sqlite3_stmt* stmt;
        const char* sql = "INSERT INTO packages (name, version, arch) VALUES (?, ?, ?);";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
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

        std::cout << "Successfully installed " << name << " v" << version << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Installation failed: " << e.what() << "\n";
        return false;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        show_help();
        return 1;
    }

    std::string command = argv[1];

    if (command == "install") {
        if (!is_superuser()) {
            std::cerr << "Error: Requires root privileges\n";
            return 1;
        }

        if (argc < 3) {
            std::cerr << "Error: Missing package file\n";
            return 1;
        }

        // Create temporary extraction directory
        std::filesystem::path temp_dir = "/var/lib/anemonix/builds/tmp_" + std::to_string(getpid());
        if (!std::filesystem::create_directory(temp_dir)) {
            std::cerr << "Error: Failed to create temp directory\n";
            return 1;
        }

        // Step 1: Untar package
        if (!untar_package(argv[2], temp_dir.string())) {
            std::filesystem::remove_all(temp_dir);
            return 1;
        }

        // Step 2: Find package root
        std::filesystem::path package_root = find_package_root(temp_dir);
        if (package_root.empty()) {
            std::filesystem::remove_all(temp_dir);
            return 1;
        }

        // Step 3: Validate package
        if (!validate_apkg(package_root)) {
            std::filesystem::remove_all(temp_dir);
            return 1;
        }

        // Step 4: Read metadata and create target directory
        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        std::string name = config["name"].as<std::string>();
        std::string version = config["version"].as<std::string>();
        std::filesystem::path target_dir = "/var/lib/anemonix/packages/" + name + "-" + version;

        // Move package to final location
        std::filesystem::rename(package_root, target_dir);
        std::filesystem::remove_all(temp_dir);

        // Step 5: Install
        if (!install_package(target_dir)) {
            std::filesystem::remove_all(target_dir);
            return 1;
        }

    } if (command == "init") {
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
        if (!install_package(package_path)) {
            return 1;
        }
    } else {
        std::cerr << "Unknown command\n";
        show_help();
        return 1;
    }

    return 0;
}