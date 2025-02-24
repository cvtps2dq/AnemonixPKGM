//
// Created by cv2 on 2/24/25.
//

#include <config.h>

#include "Filesystem.h"
#include <filesystem>
#include <iostream>

bool Filesystem::initFolders() {
    for (const auto& dir : AConf::REQUIRED_DIRS) {
        if (std::filesystem::create_directories( (AConf::ANEMO_ROOT + dir).c_str()) == 0 && errno != EEXIST) {
            std::cerr << "Error: Failed to create directory " << dir << "\n";
            return false;
        }
    }
    return true;
}
