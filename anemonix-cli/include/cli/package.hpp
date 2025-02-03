//
// Created by cv2 on 03.02.2025.
//

#ifndef PACKAGE_H
#define PACKAGE_H

#pragma once
#include <filesystem>
#include <vector>

struct PackageMetadata {
    std::string name;
    std::string version;
    std::vector<std::string> dependencies;
    std::string source_url;
};

PackageMetadata parse_package(const std::filesystem::path& pkg_path);
void install_package(const PackageMetadata& pkg, bool build_from_source);

#endif //PACKAGE_H
