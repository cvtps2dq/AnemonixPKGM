//
// Created by cv2 on 04.02.2025.
//

#ifndef UTILITIES_H
#define UTILITIES_H
#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/directory_iterator.h>
#elif defined (__linux__)
    #include <filesystem>
    #include <vector>

#endif
#include <unordered_set>
#include "Package.h"

class Utilities {
    public:
        static std::shared_ptr<Package> parseDependency(const std::string& dep_entry);
        static Package parseMetadata(const std::filesystem::path& path);
        static int compareVersions(const std::string& v1, const std::string& v2);
        static std::vector<std::string> splitString(const std::string &str);

        static bool satisfiesDependency(const std::string &dep, const Package &pkg);

        static bool runScript(const std::string &scriptPath);
};

#endif //UTILITIES_H
