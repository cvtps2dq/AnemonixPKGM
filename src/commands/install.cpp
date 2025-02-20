//
// Created by cv2 on 04.02.2025.
//

#include <iostream>
#include <unistd.h>
#include <Utilities.h>
#include <yaml-cpp/yaml.h>
#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/operations.h>
    #include <__filesystem/recursive_directory_iterator.h>
#elif defined (__linux__)
    #include <filesystem>
    #include <fmt/core.h>
    #include <vector>
#endif
#include <Database.h>

#include "colors.h"
#include "Anemo.h"
#include "config.h"
#include <fmt/ranges.h>
#include <sys/acl.h>
#include <sys/stat.h>

std::string removeExtension(const std::string& filename) {
    if (size_t last_dot = filename.rfind(".apkg"); last_dot != std::string::npos) {
        return filename.substr(0, last_dot);
    }
    return filename;  // Return original if no `.apkg`
}

void copyFileWithMetadata(const std::filesystem::path& source, const std::filesystem::path& destination) {
    try { if (!exists(destination))rename(source, destination);}
    catch (const std::exception& e) {
        std::cerr << "Error copying file " << source << " -> " << destination << ": " << e.what() << std::endl;
    }
}

int compareVersions(const std::string& v1, const std::string& v2) {
    std::vector<int> nums1, nums2;
    std::stringstream ss1(v1), ss2(v2);
    std::string temp;

    while (std::getline(ss1, temp, '.')) nums1.push_back(std::stoi(temp));
    while (std::getline(ss2, temp, '.')) nums2.push_back(std::stoi(temp));

    while (nums1.size() < nums2.size()) nums1.push_back(0);
    while (nums2.size() < nums1.size()) nums2.push_back(0);

    for (size_t i = 0; i < nums1.size(); i++) {
        if (nums1[i] < nums2[i]) return -1;
        if (nums1[i] > nums2[i]) return 1;
    }
    return 0;
}

bool Anemo::install(const std::vector<std::string>& arguments, bool force, bool reinstall) {

    if (!Utilities::isSu()) {
        std::cerr << RED << "err: requires root privileges :( \n" << RESET;
        return false;
    }

    if (arguments.empty()) {
        std::cerr << RED << "err: missing package file :( \n" << RESET;
        return false;
    }

    const std::string& package_path = arguments[0];  // First non-flag argument is the package file

    std::vector<std::pair<std::string, std::string>> provided_items;
    // Create temporary directory
    std::string temp_dir = "/tmp/anemo" + Utilities::random_string(6);
    std::filesystem::path temp_path(temp_dir);
    std::cout << temp_path << std::endl;
    if (!mkdtemp(temp_dir.data())) {
        std::cerr << "Failed to create temporary directory" << std::endl;
        return false;
    }
    std::string temp_dir_str(temp_dir);

    // First pass: Extract metadata and scripts
    std::unordered_set<std::string> metadata_files;
    if (!Utilities::extractMetadataAndScripts(package_path, temp_dir_str, metadata_files)) {
        std::filesystem::remove_all(temp_path);
        return false;
    }

    // Verify metadata exists
    std::string metadata_path = temp_dir_str + "/" + removeExtension(arguments[0]);
    std::filesystem::path root(metadata_path);
    if (access((root / "anemonix.yaml").c_str(), F_OK) != 0) {
        std::cerr << "Missing required metadata file (anemonix.yaml)" << std::endl;
        std::filesystem::remove_all(temp_path);
        return false;
    }

    std::string description = "none";
    YAML::Node config = YAML::LoadFile(root / "anemonix.yaml");
    const auto name = config["name"].as<std::string>();
    const auto version = config["version"].as<std::string>();
    const auto arch = config["arch"].as<std::string>();
    if (config["description"]) description = config["description"].as<std::string>();

    // Parse 'provides'
    std::string parent_version = config["version"] ? config["version"].as<std::string>() : "";

    if (config["provides"]) {
        for (const auto& item : config["provides"]) {
            auto provides_entry = item.as<std::string>();
            size_t pos = provides_entry.find('=');

            std::string prov_name;
            std::string prov_version;

            if (pos != std::string::npos) {
                prov_name = provides_entry.substr(0, pos);
                prov_version = provides_entry.substr(pos + 1);
            } else {
                prov_name = provides_entry;
                prov_version = parent_version;  // Use parent package version
            }

            provided_items.emplace_back(prov_name, prov_version);
        }
    }

    std::vector<std::string> dependencies;
    if (config["deps"]) {
        for (const auto& dep : config["deps"]) {
            dependencies.push_back(dep.as<std::string>());
        }
    }
    std::string deps_str = dependencies.empty() ? "" : fmt::format("{}", fmt::join(dependencies, ","));

    // Handle version comparison if already installed
    if (std::string installed_version = Database::getPkgVersion(name); !installed_version.empty()) {
        if (int cmp = compareVersions(version, installed_version); cmp > 0) {
            std::cout << YELLOW << "An update is available: " << installed_version << " -> " << version << RESET << "\n";
            std::cout << "Do you want to update? (y/n): ";
        } else if (cmp < 0) {
            std::cout << RED << "Warning: You are about to downgrade from " << installed_version << " to " << version << RESET << "\n";
            std::cout << "Do you want to proceed? (y/n): ";
        } else if (reinstall) {
            std::cout << YELLOW << "Reinstalling package: " << name << RESET << "\n";
            std::cout << "Are you sure? This will erase the current installation. (y/n): ";
        } else {
            std::cout << GREEN << "Package " << name << " is already installed and up-to-date!\n" << RESET;
            remove_all(temp_path);
            return true;
        }

        char choice;
        std::cin >> choice;
        if (choice != 'y') {
            std::cout << "Installation aborted.\n";
            remove_all(temp_path);
            return false;
        }

        // Remove old package before upgrade/downgrade/reinstall
        remove(name, force, true);
    }

    // Check for missing dependencies
    std::vector<std::string> missing_deps = Database::checkDependencies(dependencies);

    if (!force && !missing_deps.empty() && AConf::BSTRAP_PATH.empty()) {
        std::cout << RED << "\n! Installation Failed: Missing Dependencies !\n" << RESET;
        for (const auto& dep : missing_deps) {
            std::cout << "-> " << YELLOW << dep << RESET << " is not installed.\n";
        }
        std::cout << "Use --force to install anyway.\n";
        return false;
    }
    if (!missing_deps.empty()) {
        std::cout << YELLOW << "Warning: Proceeding with installation despite missing dependencies.\n" << RESET;
    }

    // User confirmation
    std::string input;
    if (AConf::BSTRAP_PATH.empty()) {
        std::cout << "Do you want to install this package? (y/n): ";
        std::getline(std::cin, input);
    } else {
        input = 'y';
    }
    if (!input.empty() && tolower(input[0]) != 'y') {
        std::cout << "Installation canceled" << std::endl;
        remove_all(temp_path);
        return false;
    }

    std::vector<std::string> installed_files;
    if (!Utilities::extractRemainingFiles(package_path,  metadata_files, installed_files)) {
        remove_all(temp_path);
        return false;
    }

    if (std::filesystem::path install_script = root / "install.anemonix"; exists(install_script)) {
        if (system(install_script.string().c_str()) != 0) {
            throw std::runtime_error("install.anemonix script failed");
        }
    }

    // Insert package into DB
    if(!Database::insertPkg(name, version, arch, deps_str, description)){
        throw std::runtime_error("failed to insert pkg into the database!");
    }

    // Store installed_files in package database or process further
    std::cout << "Installed " << installed_files.size() << " files\n";
    Database::writePkgFilesBatch(name, installed_files);

    // Insert provided items into the databases
    for (const auto& [prov_name, prov_version] : provided_items) {
        if (std::string desc = "provided by: " + name; !Database::insertPkg(prov_name, prov_version, arch, "", desc)) {
            std::cerr << RED << "Failed to insert provided package: " << prov_name << RESET << "\n";
        } else {
            std::cout << GREEN << "-> Provided package " << prov_name << "=" << prov_version << " registered successfully!\n" << RESET;
        }
    }
    // Mark package as broken if dependencies are missing
    if (!missing_deps.empty()) {
        Database::markAsBroken(name, deps_str);
    }

    std::cout << GREEN << ":) Successfully installed " << name << " v" << version << "\n" << RESET;
    try{
        remove_all(temp_path);
    } catch(std::exception& e){
        std::cout << "failed to remove package root. " << e.what() << std::endl;
    }
    remove_all(temp_path);
    return true;
}

