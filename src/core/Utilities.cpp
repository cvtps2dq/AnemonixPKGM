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

#include <Database.h>
#include "config.h"
#include <fstream>
#include <random>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <string>
#include <unordered_set>
#include <cstring>

bool Utilities::isSu() {
    return geteuid() == 0;
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
    std::unordered_set<std::string>& metadata_files) {
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
    int ix = 0;
    std::string root_path;

    while (true) {
        int r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) break;
        if (r != ARCHIVE_OK) {
            std::cerr << "Archive error: " << archive_error_string(a) << std::endl;
            status = ARCHIVE_FATAL;
            break;
        }

        std::string filename = archive_entry_pathname(entry);
        if (ix == 0) root_path = filename;
        ix++;
        std::string relative_path = filename.substr(root_path.length());
        if (!relative_path.empty() && relative_path[0] == '/') {
            relative_path = relative_path.substr(1);
        }

        if (!isMetadataOrScript(relative_path)) {
            archive_read_data_skip(a);
            continue;
        }

        metadata_files.insert(filename);
        std::filesystem::path extracted_file =
            std::filesystem::path(filename.substr(root_path.length())).lexically_normal();
        std::filesystem::path fullpath = (temp_dir / extracted_file).lexically_normal();
        archive_entry_set_pathname(entry, fullpath.c_str());
        std::cout << "fullpath: " << fullpath << std::endl;

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

    while (true) {
        int r = archive_read_next_header(a, &entry);
        if (r == ARCHIVE_EOF) break;
        if (r != ARCHIVE_OK) {
            std::cerr << "Archive error: " << archive_error_string(a) << std::endl;
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
            file_count = 1;
            continue;
        }

        if (filename.substr(root.length()) == "anemonix.yaml" || filename.substr(root.length()) == "install.anemonix" ||
            filename.substr(root.length()) == "update.anemonix" || filename.substr(root.length()) == "remove.anemonix") {
            archive_read_data_skip(a);
            continue;
        }

        std::filesystem::path base_path = AConf::BSTRAP_PATH.empty() ? "/" :
                                  std::filesystem::absolute(AConf::BSTRAP_PATH).lexically_normal();

        std::cout << "BSTRAP_PATH: " << base_path << std::endl;

        // Strip root prefix correctly without forcing absolute paths
        std::filesystem::path extracted_file = filename.substr(root.length() + 7);
        extracted_file = extracted_file.lexically_normal();

        std::filesystem::path fullpath;

        if (base_path.empty()) {
            // No bootstrap → just use extracted file as is (must be absolute)
            fullpath = "/" / extracted_file;  // Ensure absolute path
        } else {
            // Bootstrap mode → place extracted files inside bootstrap root
            std::string temp = base_path.string();
            temp.pop_back();
            fullpath = temp / extracted_file;
        }

        // bstrp: /home/test/test/
        // ext: /boot/



        // Normalize final path
        fullpath = fullpath.lexically_normal();

        std::cout << "Final Full Path: " << fullpath << std::endl;
        archive_entry_set_pathname(entry, fullpath.c_str());

        // r = archive_write_header(ext, entry);
        // if (r != ARCHIVE_OK) {
        //     std::cerr << "Write error: " << archive_error_string(ext) << std::endl;
        //     status = ARCHIVE_FATAL;
        //     break;
        // }

        // Copy data and track successful extraction
        const void* buff;
        size_t size;
        la_int64_t offset;
        bool extraction_success = true;

        // while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
        //     if (archive_write_data_block(ext, buff, size, offset) != ARCHIVE_OK) {
        //         std::cerr << "Write error: " << archive_error_string(ext) << std::endl;
        //         status = ARCHIVE_FATAL;
        //         extraction_success = false;
        //         break;
        //     }
        // }

        if (extraction_success /*&& r == ARCHIVE_EOF*/) {
            if (AConf::BSTRAP_PATH.empty()) {
                if (!is_directory(fullpath)) {
                    installed_files.push_back(fullpath);
                }
            } else {
                // Normalize BSTRAP_PATH
                std::filesystem::path bootstrap_path = std::filesystem::absolute(AConf::BSTRAP_PATH).lexically_normal();
                std::filesystem::path relative_path = std::filesystem::path(fullpath).lexically_proximate(bootstrap_path);

                // Ensure absolute path (prefix with '/')
                std::filesystem::path final_path = "/" / relative_path;

                if (!is_directory(fullpath)) {
                    installed_files.push_back(final_path);
                }
            }

            file_count++;
        }

        //archive_write_finish_entry(ext);
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



