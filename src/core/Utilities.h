//
// Created by cv2 on 04.02.2025.
//

#ifndef UTILITIES_H
#define UTILITIES_H
#include <__filesystem/path.h>

class Utilities {
    public:
        static bool isSu();
        static bool initFolders();
        static bool untarPKG(const std::string& package_path, const std::string& extract_to);
        static std::filesystem::path findPKGRoot(const std::filesystem::path& temp_dir);
};

#endif //UTILITIES_H
