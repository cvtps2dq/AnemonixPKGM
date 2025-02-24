//
// Created by cv2 on 2/24/25.
//

#include "Filesystem.h"
#include <archive.h>
#include <archive_entry.h>
#include <cerrno>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>
#include <vector>
#include <algorithm>

bool Filesystem::extractMetadataAndScripts(const std::string& package_path, const std::string& temp_dir,
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