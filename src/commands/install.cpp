//
// Created by cv2 on 04.02.2025.
//

#include <iostream>
#include <unistd.h>
#include <Utilities.h>
#include <yaml-cpp/yaml.h>
#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/operations.h>
#elif defined (__linux__)
    #include <filesystem>
    #include <vector>
#endif
#include "colors.h"
#include "Anemo.h"
#include "config.h"
#include "defines.h"

bool Anemo::installCmd(int argc, char* argv[], bool force, bool reinstall) {
    if (!Utilities::isSu()) {
            std::cerr << RED << "err: requires root privileges :( \n" << RESET;
            return false;
        }

        if (argc < 3) {
            std::cerr << RED << "err: missing package file :( \n" << RESET;
            return false;
        }

        // Create temporary extraction directory

        std::filesystem::path temp_dir = AConf::ANEMO_ROOT + "/builds/tmp_" + std::to_string(getpid());
        if (!create_directory(temp_dir)) {
            std::cerr << RED << "err: failed to create temp directory :( \n" << RESET;
            return false;
        }

        // Step 1: Untar package
        if (!Utilities::untarPKG(argv[2], temp_dir.string())) {
            remove_all(temp_dir);
            return false;
        }

        // Step 2: Find package root
        std::filesystem::path package_root = Utilities::findPKGRoot(temp_dir);
        if (package_root.empty()) {
            remove_all(temp_dir);
            return false;
        }

        // Step 3: Validate package
        if (!validate(package_root)) {
            remove_all(temp_dir);
            return false;
        }

        // Step 4: Read metadata and create target directory

        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        auto name = config["name"].as<std::string>();
        auto version = config["version"].as<std::string>();

        std::filesystem::path target_dir = AConf::ANEMO_ROOT + "/packages/" + name + "-" + version;

        // Move package to final location
        std::filesystem::rename(package_root, target_dir);
        remove_all(temp_dir);

        // Step 5: Install
        if (!install(target_dir, force, reinstall)) {
            std::cout << DSTRING << RED << "install failed :(" << std::endl << RESET;
            std::cout << DSTRING << RED << "removing package dir: " << target_dir << std::endl << RESET;
            remove_all(target_dir);
            return false;
        }
    return true;
}
