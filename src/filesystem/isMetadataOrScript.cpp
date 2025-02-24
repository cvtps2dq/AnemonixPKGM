//
// Created by cv2 on 2/24/25.
//

#include <cstring>
#include <ranges>

#include "Filesystem.h"

bool Filesystem::isMetadataOrScript(const std::string& entry_name) {
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