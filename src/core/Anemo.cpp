//
// Created by cv2 on 04.02.2025.
//

#include "Anemo.h"

#include <colors.h>
#include <iostream>
#include <sqlite3.h>
#include <unordered_set>
#include <yaml-cpp/yaml.h>

#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/operations.h>
    #include <__filesystem/recursive_directory_iterator.h>
#elif defined (__linux__)
    #include <filesystem>
    #include <vector>
#endif

#include <random>

#include "config.h"
#include <fmt/format.h>
#include <fmt/ranges.h>

bool Anemo::validate(const std::filesystem::path& package_root) {
    const std::unordered_set<std::string> REQUIRED_FILES = {
        "build.anemonix",
        "anemonix.yaml",
        "package/"
    };

    for (const auto& item : REQUIRED_FILES) {
        if (std::filesystem::path path = package_root / item; !exists(path)) {
            std::cerr << "err: missing required package item: " << item << "\n";
            return false;
        }
    }
    return true;
}

void Anemo::showHelp() {
    std::cout << "anemo package manager\n";
    std::cout << "usage: anemo <command> [options]\n\n";
    std::cout << "commands:\n";
    std::cout << "  init            initialize anemo folders and database\n";
    std::cout << "  install <pkg>   install a package\n";
    std::cout << "  remove <pkg>    remove a package\n";
    std::cout << "  update          update package lists\n";
    std::cout << "  build <pkg>     build a package from source\n";
    std::cout << "  search <query>  search for a package\n";
    std::cout << "  audit           verify installed packages\n";
}




