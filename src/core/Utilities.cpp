//
// Created by cv2 on 04.02.2025.
//

#include "Utilities.h"

#include <archive.h>
#include <archive_entry.h>
#include <cerrno>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>

#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/directory_iterator.h>
    #include <__filesystem/operations.h>
    #include <__algorithm/ranges_any_of.h>
#elif defined (__linux__)
    #include <filesystem>
    #include <vector>
    #include <algorithm>
#endif

#include <colors.h>
#include <Database.h>
#include "config.h"
#include <fstream>
#include <random>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <string>
#include <unordered_set>
#include <cstring>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

#include "Package.h"

bool Utilities::isSu() {
    return geteuid() == 0;
}

std::shared_ptr<Package> Utilities::parseDependency(const std::string& dep_entry) {
    std::string name, version, constraint_type;

    // Check for >= constraint
    if (dep_entry.find(">=") != std::string::npos) {
        size_t pos = dep_entry.find(">=");
        name = dep_entry.substr(0, pos);
        version = "," +dep_entry.substr(pos + 2);
        constraint_type = "greater";  // Indicates >= constraint
    }
    // Check for <= constraint
    else if (dep_entry.find("<=") != std::string::npos) {
        size_t pos = dep_entry.find("<=");
        name = dep_entry.substr(0, pos);
        version = "," + dep_entry.substr(pos + 2);
        constraint_type = "less";  // Indicates <= constraint
    }
    // Check for strict =
    else if (dep_entry.find('=') != std::string::npos) {
        size_t pos = dep_entry.find('=');
        name = dep_entry.substr(0, pos);
        version = "," + dep_entry.substr(pos + 1);
        constraint_type = "strict";  // Indicates exact version match
    }
    // No version specified, assume latest
    else {
        name = dep_entry;
        version = "";
        constraint_type = "any";  // No specific version constraint
    }

    return std::make_shared<Package>(name, constraint_type + version, "", "", "");
}

Package Utilities::parseMetadata(const std::filesystem::path& path) {
    YAML::Node config;
    std::string name;
    std::string version;
    std::string author = "unknown";
    std::string description = "no description provided";
    std::string arch;

    std::vector<std::shared_ptr<Package>> deps;
    std::vector<std::shared_ptr<Package>> conflicts;
    std::vector<std::shared_ptr<Package>> replaces;
    std::vector<std::shared_ptr<Package>> provides;

    try {
        config = YAML::LoadFile(path);
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to open metadata file" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        name = config["name"].as<std::string>();
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse package name" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        version = config["version"].as<std::string>();
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse version" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        arch = config["arch"].as<std::string>();
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse package architecture" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["description"]) {
            description = config["description"].as<std::string>();
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse description" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["author"]) {
            author = config["author"].as<std::string>();
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse package author" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        std::string parent_version = config["version"].IsDefined() ? config["version"].as<std::string>() : "";

        if (config["provides"]) {
            for (const auto& item : config["provides"]) {
                auto provides_entry = item.as<std::string>();
                size_t pos = provides_entry.find('=');

                std::string prov_name, prov_version;

                if (pos != std::string::npos) {
                    prov_name = provides_entry.substr(0, pos);
                    prov_version = provides_entry.substr(pos + 1);
                } else {
                    prov_name = provides_entry;
                    prov_version = parent_version;
                }

                auto provided = std::make_shared<Package>(prov_name, prov_version, author, "provided by: " + name, arch);
                provides.push_back(provided);
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse provided packages" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["conflicts"]) {
            for (const auto& item : config["conflicts"]) {
                conflicts.push_back(std::make_shared<Package>(item.as<std::string>(), "any", "", "", arch));
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse conflicting packages" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["deps"]) {
            for (const auto& dep : config["deps"]) {
                deps.push_back(parseDependency(dep.as<std::string>()));
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse dependencies" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["replaces"]) {
            for (const auto& item : config["replaces"]) {
                replaces.push_back(std::make_shared<Package>(item.as<std::string>(), "any", "", "", arch));
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse replacement list" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    return {name, version, author, description, arch, provides, deps, conflicts, replaces};
}

bool Utilities::initFolders() {
    for (const auto& dir : AConf::REQUIRED_DIRS) {
        if (std::filesystem::create_directories( (AConf::ANEMO_ROOT + dir).c_str()) == 0 && errno != EEXIST) {
            std::cerr << "Error: Failed to create directory " << dir << "\n";
            return false;
        }
    }
    return true;
}

std::string Utilities::random_string(const std::size_t length)
{
    const std::string CHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution distribution(0, static_cast<int>(CHARACTERS.size()) - 1);

    std::string random_string;

    for (std::size_t i = 0; i < length; ++i)
    {
        random_string += CHARACTERS[distribution(generator)];
    }

    return random_string;
}

bool Utilities::extractMetadataAndScripts(const std::string& package_path, const std::string& temp_dir,
    std::unordered_set<std::string>& metadata_files, const std::string& name) {
    archive* a = archive_read_new();
    archive_read_support_format_tar(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, package_path.c_str(), 10240) != ARCHIVE_OK) {
        std::cerr << "Error opening package: " << archive_error_string(a) << std::endl;
        return false;
    }

    archive* ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM);

    archive_entry* entry;
    int status = ARCHIVE_OK;

    while (true) {
        int r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) break;
        if (r != ARCHIVE_OK) {
            std::cerr << "Archive error: " << archive_error_string(a) << std::endl;
            status = ARCHIVE_FATAL;
            break;
        }

        std::string filename = archive_entry_pathname(entry);
        if (!isMetadataOrScript(std::filesystem::path(filename).lexically_normal())) {
            archive_read_data_skip(a);
            continue;
        }

        metadata_files.insert(filename);
        std::filesystem::path extracted_file = std::filesystem::path(filename).lexically_normal();
        std::filesystem::path fullpath = (temp_dir / extracted_file).lexically_normal();
        archive_entry_set_pathname(entry, fullpath.c_str());
        //std::cout << "fullpath: " << fullpath << std::endl;

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            std::cerr << "Write error: " << archive_error_string(ext) << std::endl;
            status = ARCHIVE_FATAL;
            break;
        }

        const void* buff;
        size_t size;
        la_int64_t offset;
        while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
            if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
                std::cerr << "Write error: " << archive_error_string(ext) << std::endl;
                status = ARCHIVE_FATAL;
                break;
            }
        }
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return status == ARCHIVE_OK;
}

bool Utilities::extractRemainingFiles(const std::string& package_path,
                                     const std::unordered_set<std::string>& exclude_files,
                                     std::vector<std::string>& installed_files) {
    archive* a = archive_read_new();
    archive_read_support_format_tar(a);
    archive_read_support_filter_all(a);

    if (archive_read_open_filename(a, package_path.c_str(), 10240) != ARCHIVE_OK) {
        std::cerr << "Error opening package: " << archive_error_string(a) << std::endl;
        return false;
    }

    archive* ext = archive_write_disk_new();
    archive_write_disk_set_options(ext,
        ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM |
        ARCHIVE_EXTRACT_ACL | ARCHIVE_EXTRACT_XATTR | ARCHIVE_EXTRACT_OWNER);

    archive_entry* entry;
    int status = ARCHIVE_OK;
    constexpr char spin_chars[] = {'|', '/', '-', '\\'};
    int spin_index = 0;
    int file_count = 0;
    std::string root;

    std::vector<std::pair<std::filesystem::path, std::string>> hard_links;

    // First pass: Extract regular files
    while (true) {
        int r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) break;
        if (r != ARCHIVE_OK) {
            std::cerr << RED << "Archive error: " << archive_error_string(a) << RESET << std::endl;
            status = ARCHIVE_FATAL;
            break;
        }

        std::string filename = archive_entry_pathname(entry);

        if (exclude_files.contains(filename)) {
            archive_read_data_skip(a);
            continue;
        }

        // Update progress spinner
        if (file_count % 4 == 0) {
            std::cout << "\r[ " << spin_chars[spin_index] << " ] Extracting files...";
            std::cout.flush();
            spin_index = (spin_index + 1) % 4;
        }
        if (file_count == 0) {
            root = filename;
            file_count++;
            continue;
        }

        if (!(filename.starts_with("package/") || filename.starts_with("./package/"))) {
            archive_read_data_skip(a);
            continue;
        }

        std::filesystem::path base_path = AConf::BSTRAP_PATH.empty() ? "/" :
                                  std::filesystem::absolute(AConf::BSTRAP_PATH).lexically_normal();

        std::filesystem::path extracted_file;
        try {
            std::string sanitized = filename;

            // Remove "package/" or "./package/" prefix if present
            if (sanitized.starts_with("package/")) {
                sanitized = sanitized.substr(8); // Cut "package/"
            } else if (sanitized.starts_with("./package/")) {
                sanitized = sanitized.substr(10); // Cut "./package/"
            }

            // Ensure it's a valid path
            extracted_file = std::filesystem::path(sanitized).lexically_normal();
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            std::cerr << "root path: " << root << std::endl;
            std::cerr << "Failed to determine the path of file: " << filename << std::endl;
            exit(EXIT_FAILURE);
            continue;
        }

        extracted_file = extracted_file.lexically_normal();
        std::filesystem::path fullpath = base_path / extracted_file;

        fullpath = fullpath.lexically_normal();

        if (archive_entry_hardlink(entry)) {
            std::string target = archive_entry_hardlink(entry);
            hard_links.emplace_back(fullpath, target);
            continue;
        }

        archive_entry_set_pathname(entry, fullpath.c_str());

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            std::cerr << "Write error: " << archive_error_string(ext) << std::endl;
            status = ARCHIVE_FATAL;
            break;
        }

        if (!std::filesystem::exists(fullpath)) {
            std::cerr << YELLOW << "File vanished: " << fullpath << RESET << std::endl;
        }

        // Copy data and track successful extraction
        const void* buff;
        size_t size;
        la_int64_t offset;
        bool extraction_success = true;

        while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
            if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
                std::cerr << "Write error: " << archive_error_string(ext) << std::endl;
                status = ARCHIVE_FATAL;
                extraction_success = false;
                break;
            }
        }

        if (extraction_success) {
            std::filesystem::path relative_path = extracted_file.lexically_normal();

            std::filesystem::path final_path = "/" / relative_path;

            if (!is_directory(fullpath)) {
                installed_files.push_back(final_path);
            }

            file_count++;
        }
    }

    // Second pass: Process hard links
    for (auto& [link_path, target_path] : hard_links) {
        std::filesystem::path full_target = AConf::BSTRAP_PATH.empty() ? std::filesystem::path("/") / target_path
                                                                       : std::filesystem::absolute(AConf::BSTRAP_PATH) / target_path;

        if (std::filesystem::exists(full_target)) {
            try {
                std::filesystem::create_hard_link(full_target, link_path);
            } catch (const std::exception& e) {
                std::cerr << YELLOW << "Failed to create hard link: " << link_path << " → " << full_target << "\n";
                std::cerr << "Exception: " << e.what() << RESET << std::endl;
            }
        }
    }

    std::cout << "\r[ OK ] Extraction complete.                            \n";
    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);

    return status == ARCHIVE_OK;
}


bool Utilities::isMetadataOrScript(const std::string& entry_name) {
    const char* targets[] = {
        "anemonix.yaml",
        "install.anemonix",
        "update.anemonix",
        "remove.anemonix"
    };

    return std::ranges::any_of(targets, [entry_name](const char* target) {
        return strcmp(entry_name.c_str(), target) == 0;
    });
}

std::filesystem::path Utilities::findPKGRoot(const std::filesystem::path &temp_dir) {
    // Check if metadata exists directly in temp dir
    if (exists(temp_dir / "anemonix.yaml")) {
        return temp_dir;
    }

    // Look for a single subdirectory containing metadata
    std::filesystem::path package_root;
    for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
        if (entry.is_directory() && exists(entry.path() / "anemonix.yaml")) {
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
