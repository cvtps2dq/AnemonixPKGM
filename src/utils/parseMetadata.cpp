//
// Created by cv2 on 2/24/25.
//

#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>

#include "Utilities.h"

Package Utilities::parseMetadata(const std::filesystem::path& path) {
    YAML::Node config;
    std::string name, version, arch;
    std::string author = "unknown";
    std::string description = "no description provided";
    bool is_protected = false;

    std::vector<std::string> deps;
    std::vector<std::string> conflicts;
    std::vector<std::string> replaces;
    std::vector<std::string> provides;

    try {
        config = YAML::LoadFile(path);
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] Failed to open metadata file: " << path << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        name = config["name"].as<std::string>();
        version = config["version"].as<std::string>();
        arch = config["arch"].as<std::string>();

        if (config["description"]) {
            description = config["description"].as<std::string>();
        }
        if (config["author"]) {
            author = config["author"].as<std::string>();
        }
        if (config["protected"]) {
            is_protected = config["protected"].as<bool>();
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] Failed to parse required fields in metadata" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["provides"]) {
            for (const auto& item : config["provides"]) {
                provides.push_back(item.as<std::string>());
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] Failed to parse provided packages" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["conflicts"]) {
            for (const auto& item : config["conflicts"]) {
                conflicts.push_back(item.as<std::string>());
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] Failed to parse conflicts" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["deps"]) {
            for (const auto& item : config["deps"]) {
                deps.push_back(item.as<std::string>());
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] Failed to parse dependencies" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["replaces"]) {
            for (const auto& item : config["replaces"]) {
                replaces.push_back(item.as<std::string>());
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] Failed to parse replacements" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    return {name, version, author, description, arch, provides, deps, conflicts, replaces, is_protected};
}