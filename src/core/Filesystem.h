//
// Created by cv2 on 2/24/25.
//

#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <unordered_set>
#include <bits/basic_string.h>
#include <vector>

#endif //FILESYSTEM_H

class Filesystem {
    static bool extractMetadataAndScripts(const std::string &package_path, const std::string &temp_dir,
                                          std::unordered_set<std::string> &metadata_files, const std::string &name);

    static bool isMetadataOrScript(const std::string &entry_name);

    static bool extractRemainingFiles(const std::string &package_path, const std::unordered_set<std::string> &exclude_files,
                               std::vector<std::string> &installed_files);

    static bool initFolders();
};