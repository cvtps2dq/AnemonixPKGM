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
    const std::vector<std::string> REQUIRED_DIRS = {
        "/cache",
        "/builds",
        "/packages"
    };

    inline std::string ANEMO_ROOT = "/var/lib/anemonix/";
    inline std::string DB_PATH =  "/var/lib/anemonix/installed.db";
    inline std::string BSTRAP_PATH;

};

#endif //CONFIG_H
