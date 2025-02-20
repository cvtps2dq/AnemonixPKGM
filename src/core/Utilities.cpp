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
        std::cout << filename << std::endl;

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
        if (!filename.starts_with(root + "package/")) {
            archive_read_data_skip(a);
            continue;
        }

        std::filesystem::path base_path = AConf::BSTRAP_PATH.empty() ? "/" :
                                  std::filesystem::absolute(AConf::BSTRAP_PATH).lexically_normal();

        std::filesystem::path extracted_file;
        try {
            extracted_file = filename.substr(root.length() + 7);
        } catch (const std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            std::cerr << "Failed to determine the path of file: " << filename << std::endl;
            continue;
        }

        extracted_file = extracted_file.lexically_normal();
        std::filesystem::path fullpath;

        if (base_path.empty()) {
            fullpath = "/" / extracted_file;  // Ensure absolute path
        } else {
            try {
                fullpath = base_path / extracted_file.string().substr(1, std::string::npos);
            } catch (const std::exception& e) {
                std::cerr << "Exception: " << e.what() << std::endl;
                std::cerr << "Failed to determine fullpath for: " << filename << std::endl;
                continue;
            }
        }

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
            if (AConf::BSTRAP_PATH.empty()) {
                if (!is_directory(fullpath)) {
                    installed_files.push_back(fullpath);
                }
            } else {
                std::filesystem::path bootstrap_path = std::filesystem::absolute(AConf::BSTRAP_PATH).lexically_normal();
                std::filesystem::path relative_path = std::filesystem::path(fullpath).lexically_proximate(bootstrap_path);
                std::filesystem::path final_path = "/" / relative_path;

                if (!is_directory(fullpath)) {
                    installed_files.push_back(final_path);
                }
            }

            file_count++;
        }

        archive_write_finish_entry(ext);
    }

    // Second pass: Process hard links
    for (auto& [link_path, target_path] : hard_links) {
        std::filesystem::path full_target = AConf::BSTRAP_PATH.empty() ? std::filesystem::path("/") / target_path
                                                                       : std::filesystem::absolute(AConf::BSTRAP_PATH) / target_path;

        if (std::filesystem::exists(full_target)) {
            try {
                std::filesystem::create_hard_link(full_target, link_path);
            } catch (const std::exception& e) {
                std::cerr << "Failed to create hard link: " << link_path << " → " << full_target << "\n";
                std::cerr << "Exception: " << e.what() << std::endl;
            }
        } else {
            std::cerr << "Hard-link target does not exist: " << full_target << "\n";
        }
    }

    std::cout << "\r[ OK ] Extraction complete.                            \n";
    system("ls /usr/bin | grep systemd");

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



