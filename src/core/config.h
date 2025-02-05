//
// Created by cv2 on 04.02.2025.
//

#ifndef CONFIG_H
#define CONFIG_H
#include <string>

#if defined (__linux__)
#include <vector>
#endif

namespace AConf {
    const std::string DB_PATH = "/var/lib/anemonix/installed.db";
    const std::vector<std::string> REQUIRED_DIRS = {
        "/var/lib/anemonix",
        "/var/lib/anemonix/cache",
        "/var/lib/anemonix/builds",
        "/var/lib/anemonix/packages"
    };

}

#endif //CONFIG_H
