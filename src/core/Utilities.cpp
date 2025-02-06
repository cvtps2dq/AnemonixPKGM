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
#elif defined (__linux__)
    #include <filesystem>
    #include <vector>
#endif

#include "config.h"
#include <fstream>

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

bool Utilities::untarPKG(const std::string& package_path, const std::string& extract_to) {
    struct archive* a;
    struct archive* ext;
    struct archive_entry* entry;
    int r;

    const char spin_chars[] = {'|', '/', '-', '\\'};
    int spin_index = 0;

    // Open the tar archive for reading
    a = archive_read_new();
    archive_read_support_format_tar(a);
    archive_read_support_filter_all(a);

    if ((archive_read_open_filename(a, package_path.c_str(), 10240) != ARCHIVE_OK)) {
        std::cerr << "Error opening archive: " << archive_error_string(a) << std::endl;
        return false;
    }

    // Create an archive for extraction
    ext = archive_write_disk_new();
    archive_write_disk_set_options(ext, ARCHIVE_EXTRACT_TIME |  // Preserve timestamps
                                         ARCHIVE_EXTRACT_PERM |  // Preserve permissions
                                         ARCHIVE_EXTRACT_ACL |   // Preserve ACLs
                                         ARCHIVE_EXTRACT_XATTR | // Preserve xattrs
                                         ARCHIVE_EXTRACT_OWNER); // Preserve ownership

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* entry_name = archive_entry_pathname(entry);
        std::cout << "Extracting: " << entry_name << std::endl;
        // Animate while extracting
        std::cout << "\r[ " << spin_chars[spin_index] << " ] Extracting: " << entry_name << std::flush;

        spin_index = (spin_index + 1) % 4;

        // Set output directory
        std::string full_path = std::string(extract_to) + "/" + entry_name;
        archive_entry_set_pathname(entry, full_path.c_str());

        r = archive_write_header(ext, entry);
        if (r != ARCHIVE_OK) {
            std::cerr << "Error writing header: " << archive_error_string(ext) << std::endl;
        } else {
            const void* buff;
            size_t size;
            la_int64_t offset;

            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                archive_write_data_block(ext, buff, size, offset);
            }

            if (r != ARCHIVE_EOF) {
                std::cerr << "Error extracting file: " << archive_error_string(a) << std::endl;
            }
        }

        archive_write_finish_entry(ext);
    }

    archive_read_close(a);
    archive_read_free(a);
    archive_write_close(ext);
    archive_write_free(ext);
    return true;
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



