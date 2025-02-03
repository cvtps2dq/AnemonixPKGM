//
// Created by cv2 on 03.02.2025.
//

// package.hpp


// package.cpp
#include "cli/package.hpp"
#include <yaml-cpp/yaml.h>

PackageMetadata parse_package(const std::filesystem::path& pkg_path) {
    PackageMetadata meta;
    const auto yaml_file = pkg_path / "anemonix.yaml";

    YAML::Node config = YAML::LoadFile(yaml_file.string());
    meta.name = config["name"].as<std::string>();
    meta.version = config["version"].as<std::string>();

    if (config["deps"])
        meta.dependencies = config["deps"].as<std::vector<std::string>>();

    return meta;
}