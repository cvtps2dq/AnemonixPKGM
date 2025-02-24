//
// Created by cv2 on 24.02.2025.
//

#include "colors.h"
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/node.h>
#include <yaml-cpp/node/parse.h>
#include <yaml-cpp/yaml.h>
#include <__filesystem/path.h>
#include "Package.h"

std::shared_ptr<Package> parseDependency(const std::string& dep_entry) {
    std::string name, version, constraint_type;

    // Check for >= constraint
    if (dep_entry.find(">=") != std::string::npos) {
        size_t pos = dep_entry.find(">=");
        name = dep_entry.substr(0, pos);
        version = "," +dep_entry.substr(pos + 2);
        constraint_type = "greater";  // Indicates >= constraint
    }
    // Check for <= constraint
    else if (dep_entry.find("<=") != std::string::npos) {
        size_t pos = dep_entry.find("<=");
        name = dep_entry.substr(0, pos);
        version = "," + dep_entry.substr(pos + 2);
        constraint_type = "less";  // Indicates <= constraint
    }
    // Check for strict =
    else if (dep_entry.find('=') != std::string::npos) {
        size_t pos = dep_entry.find('=');
        name = dep_entry.substr(0, pos);
        version = "," + dep_entry.substr(pos + 1);
        constraint_type = "strict";  // Indicates exact version match
    }
    // No version specified, assume latest
    else {
        name = dep_entry;
        version = "";
        constraint_type = "any";  // No specific version constraint
    }

    return std::make_shared<Package>(name, constraint_type + version, "", "", "");
}

Package parseMetadata(const std::filesystem::path& path) {
    YAML::Node config;
    std::string name;
    std::string version;
    std::string author = "unknown";
    std::string description = "no description provided";
    std::string arch;

    std::vector<std::shared_ptr<Package>> deps;
    std::vector<std::shared_ptr<Package>> conflicts;
    std::vector<std::shared_ptr<Package>> replaces;
    std::vector<std::shared_ptr<Package>> provides;

    try {
        config = YAML::LoadFile(path);
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to open metadata file" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        name = config["name"].as<std::string>();
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse package name" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        version = config["version"].as<std::string>();
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse version" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        arch = config["arch"].as<std::string>();
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse package architecture" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["description"]) {
            description = config["description"].as<std::string>();
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse description" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["author"]) {
            author = config["author"].as<std::string>();
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse package author" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        std::string parent_version = config["version"].IsDefined() ? config["version"].as<std::string>() : "";

        if (config["provides"]) {
            for (const auto& item : config["provides"]) {
                auto provides_entry = item.as<std::string>();
                size_t pos = provides_entry.find('=');

                std::string prov_name, prov_version;

                if (pos != std::string::npos) {
                    prov_name = provides_entry.substr(0, pos);
                    prov_version = provides_entry.substr(pos + 1);
                } else {
                    prov_name = provides_entry;
                    prov_version = parent_version;
                }

                auto provided = std::make_shared<Package>(prov_name, prov_version, author, "provided by: " + name, arch);
                provides.push_back(provided);
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse provided packages" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["conflicts"]) {
            for (const auto& item : config["conflicts"]) {
                conflicts.push_back(std::make_shared<Package>(item.as<std::string>(), "any", "", "", arch));
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse conflicting packages" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["deps"]) {
            for (const auto& dep : config["deps"]) {
                deps.push_back(parseDependency(dep.as<std::string>()));
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse dependencies" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    try {
        if (config["replaces"]) {
            for (const auto& item : config["replaces"]) {
                replaces.push_back(std::make_shared<Package>(item.as<std::string>(), "any", "", "", arch));
            }
        }
    } catch (const YAML::Exception&) {
        std::cerr << RED << "[ERROR] failed to parse replacement list" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    return {name, version, author, description, arch, provides, deps, conflicts, replaces};
}

int main() {
    Package test = parseMetadata("/Users/cv2/AnemonixPKGM/tests/parsing/anemonix.yaml");
    test.print();
}