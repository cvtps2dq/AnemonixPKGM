//
// Created by cv2 on 03.02.2025.
//

// commands/install.cpp
#include "../../include/cli/cli.h"
#include "../../include/cli/package.hpp"
#include <cstdlib>
#include <iostream>

void anemonix::Cli::handle_install(int argc, char** argv) {
    if (argc < 3) {
        throw std::runtime_error("Missing package name");
    }

    const std::string pkg_name = argv[2];
    bool build_from_source = false;

    // Check for --build flag
    for (int i = 3; i < argc; ++i) {
        if (std::string(argv[i]) == "--build") {
            build_from_source = true;
        }
    }

    std::cout << "Installing package: " << pkg_name << "\n";

    // TODO: Actual package resolution
    PackageMetadata fake_pkg{
        .name = pkg_name,
        .version = "1.0.0",
        .dependencies = {},
        .source_url = "https://example.com/pkg.tar.gz"
    };

    install_package(fake_pkg, build_from_source);
}

void install_package(const PackageMetadata& pkg, bool build_from_source) {
    std::cout << "Installing " << pkg.name << " v" << pkg.version << "\n";

    if (build_from_source) {
        std::cout << "Building from source...\n";
        // TODO: Actual build process
        system("echo 'Simulating build process'");
    } else {
        std::cout << "Downloading binary...\n";
        // TODO: Binary download logic
    }

    std::cout << "Package installed successfully!\n";
}