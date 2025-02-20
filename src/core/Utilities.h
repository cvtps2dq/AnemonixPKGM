//
// Created by cv2 on 04.02.2025.
//

#ifndef UTILITIES_H
#define UTILITIES_H
#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/directory_iterator.h>
#elif defined (__linux__)
    #include <filesystem>
#endif
#include <unordered_set>

class Utilities {
    public:
        static bool isSu();
        static bool initFolders();

        static std::string random_string(std::size_t length);

        static bool untarPKG(const std::string &package_path, const std::string &extract_to, bool reinstall, bool force);
        static std::filesystem::path findPKGRoot(const std::filesystem::path& temp_dir);
        static bool extractMetadataAndScripts(const std::string &package_path, const std::string &temp_dir, std::unordered_set<std::string> &metadata_files);
        static bool extractRemainingFiles(const std::string &package_path, const std::unordered_set<std::string> &exclude_files, std::vector<std::string> &
                                          installed_files);
        static bool isMetadataOrScript(const std::string &entry_name);
        static void deleteTempDir(const std::string& path);
};

#endif //UTILITIES_H
