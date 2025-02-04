//
// Created by cv2 on 04.02.2025.
//

#include <iostream>
#include <unistd.h>
#include <Utilities.h>
#include <yaml-cpp/yaml.h>
#include <__filesystem/operations.h>
#include "colors.h"
#include "Anemo.h"
#include "defines.h"

bool Anemo::installCmd(int argc, char* argv[]) {
    if (!Utilities::isSu()) {
            std::cerr << RED << "err: requires root privileges :( \n" << RESET;
            return false;
        }

        if (argc < 3) {
            std::cerr << RED << "err: missing package file :( \n" << RESET;
            return false;
        }

        // Create temporary extraction directory
        std::cout << DSTRING << CYAN << "creating temporary directory for extraction " <<
            "/var/lib/anemonix/builds/tmp_" + std::to_string(getpid()) << "\n" << RESET;

        std::filesystem::path temp_dir = "/var/lib/anemonix/builds/tmp_" + std::to_string(getpid());
        if (!create_directory(temp_dir)) {
            std::cerr << RED << "err: failed to create temp directory :( \n" << RESET;
            return false;
        }

        std::cout << DSTRING << CYAN << "unpacking package..." << std::endl << RESET;
        // Step 1: Untar package
        if (!Utilities::untarPKG(argv[2], temp_dir.string())) {
            remove_all(temp_dir);
            return false;
        }

        // Step 2: Find package root
        std::cout << DSTRING << CYAN << "finding package root..." << std::endl << RESET;
        std::filesystem::path package_root = Utilities::findPKGRoot(temp_dir);
        std::cout << DSTRING << GREEN << "found. " << package_root.c_str() << std::endl << RESET;
        if (package_root.empty()) {
            remove_all(temp_dir);
            return false;
        }

        // Step 3: Validate package
        std::cout << DSTRING << CYAN << "validating package... " << std::endl << RESET;
        if (!validate(package_root)) {
            remove_all(temp_dir);
            return false;
        }

        // Step 4: Read metadata and create target directory
        std::cout << DSTRING << CYAN << "reading package metadata... " << std::endl << RESET;
        std::cout << DSTRING << CYAN << "yaml path: " << package_root / "anemonix.yaml" << std::endl << RESET;

        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        auto name = config["name"].as<std::string>();
        auto version = config["version"].as<std::string>();

        std::cout << DSTRING << GREEN << "ok. " << "name : " << name << " version: " << version << std::endl << RESET;

        std::cout << DSTRING << CYAN << "creating target dir..." << std::endl << RESET;
        std::filesystem::path target_dir = "/var/lib/anemonix/packages/" + name + "-" + version;
        std::cout << DSTRING << GREEN << "ok. " << "/var/lib/anemonix/packages/" + name + "-" + version << std::endl;

        // Move package to final location
        std::cout << DSTRING << CYAN << "moving package to final dir..." << std::endl << RESET;
        std::filesystem::rename(package_root, target_dir);
        remove_all(temp_dir);
        std::cout << DSTRING << GREEN << "ok." << std::endl << RESET;

        // Step 5: Install
        std::cout << DSTRING << CYAN << "preparing to install package..." << std::endl << RESET;
        if (!install(target_dir)) {
            std::cout << DSTRING << RED << "install failed :(" << std::endl << RESET;
            std::cout << DSTRING << RED << "removing package dir: " << target_dir << std::endl << RESET;
            remove_all(target_dir);
            return false;
        }
    return true;
}
