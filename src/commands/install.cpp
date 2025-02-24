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
    try{
        if (const size_t last_dot = filename.rfind(".apkg"); last_dot != std::string::npos) {
            return filename.substr(0, last_dot);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << "called from removeExtension" << std::endl;
        std::cerr << filename << std::endl;
    }
    return filename;  // Return original if no `.apkg`
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


// bool Anemo::install(const std::vector<std::string>& arguments, bool force, bool reinstall) {
//     if (!Utilities::isSu()) {
//         std::cerr << RED << "err: requires root privileges :( \n" << RESET;
//         return false;
//     }
//
//     if (arguments.empty()) {
//         std::cerr << RED << "err: missing package file :( \n" << RESET;
//         return false;
//     }
//
//     const std::string& package_path = arguments[0];  // First non-flag argument is the package file
//
//     std::vector<std::pair<std::string, std::string>> provided_items;
//     // Create temporary directory
//     std::string temp_dir = "/tmp/anemoXXXXXX";
//     std::filesystem::path temp_path(temp_dir);
//     if (!mkdtemp(temp_dir.data())) {
//         std::cerr << "Failed to create temporary directory" << std::endl;
//         return false;
//     }
//     std::string temp_dir_str(temp_dir);
//     // First pass: Extract metadata and scripts
//     std::unordered_set<std::string> metadata_files;
//     if (!Utilities::extractMetadataAndScripts(package_path, temp_dir_str, metadata_files, arguments[0])) {
//         remove_all(temp_path);
//         return false;
//     }
//     // Verify metadata exists
//     std::string metadata_path = temp_dir_str + "/";
//     std::filesystem::path root(metadata_path);
//     if (access((root / "anemonix.yaml").c_str(), F_OK) != 0) {
//         std::cerr << "Missing required metadata file (anemonix.yaml)" << std::endl;
//         remove_all(temp_path);
//         return false;
//     }
//
//     std::string description = "none";
//     YAML::Node config = YAML::LoadFile(root / "anemonix.yaml");
//     const auto name = config["name"].as<std::string>();
//     const auto version = config["version"].as<std::string>();
//     const auto arch = config["arch"].as<std::string>();
//     if (config["description"]) description = config["description"].as<std::string>();
//
//     // Parse 'provides'
//     std::string parent_version = config["version"] ? config["version"].as<std::string>() : "";
//
//     if (config["provides"]) {
//         for (const auto& item : config["provides"]) {
//             auto provides_entry = item.as<std::string>();
//             size_t pos = provides_entry.find('=');
//
//             std::string prov_name;
//             std::string prov_version;
//
//             if (pos != std::string::npos) {
//                 prov_name = provides_entry.substr(0, pos);
//                 prov_version = provides_entry.substr(pos + 1);
//             } else {
//                 prov_name = provides_entry;
//                 prov_version = parent_version;  // Use parent package version
//             }
//
//             provided_items.emplace_back(prov_name, prov_version);
//         }
//     }
//
//     std::vector<std::string> dependencies;
//     if (config["deps"]) {
//         for (const auto& dep : config["deps"]) {
//             dependencies.push_back(dep.as<std::string>());
//         }
//     }
//     std::string deps_str = dependencies.empty() ? "" : fmt::format("{}", fmt::join(dependencies, ","));
//
//     // Handle version comparison if already installed
//     if (std::string installed_version = Database::getPkgVersion(name); !installed_version.empty()) {
//         if (int cmp = compareVersions(version, installed_version); cmp > 0) {
//             std::cout << YELLOW << "An update is available: " << installed_version << " -> " << version << RESET << "\n";
//             std::cout << "Do you want to update? (y/n): ";
//         } else if (cmp < 0) {
//             std::cout << RED << "Warning: You are about to downgrade from " << installed_version << " to " << version << RESET << "\n";
//             std::cout << "Do you want to proceed? (y/n): ";
//         } else if (reinstall) {
//             std::cout << YELLOW << "Reinstalling package: " << name << RESET << "\n";
//             std::cout << "Are you sure? This will erase the current installation. (y/n): ";
//         } else {
//             std::cout << GREEN << "Package " << name << " is already installed and up-to-date!\n" << RESET;
//             remove_all(temp_path);
//             return true;
//         }
//
//         char choice;
//         std::cin >> choice;
//         if (choice != 'y') {
//             std::cout << "Installation aborted.\n";
//             remove_all(temp_path);
//             return false;
//         }
//
//         // Remove old package before upgrade/downgrade/reinstall
//         remove(name, force, true);
//     }
//
//     // Check for missing dependencies
//     std::vector<std::string> missing_deps = Database::checkDependencies(dependencies);
//
//     if (!force && !missing_deps.empty() && AConf::BSTRAP_PATH.empty()) {
//         std::cout << RED << "\n! Installation Failed: Missing Dependencies !\n" << RESET;
//         for (const auto& dep : missing_deps) {
//             std::cout << "-> " << YELLOW << dep << RESET << " is not installed.\n";
//         }
//         std::cout << "Use --force to install anyway.\n";
//         return false;
//     }
//     if (!missing_deps.empty()) {
//         std::cout << YELLOW << "Warning: Proceeding with installation despite missing dependencies.\n" << RESET;
//     }
//
//     // User confirmation
//     std::string input;
//     if (AConf::BSTRAP_PATH.empty()) {
//         std::cout << "Do you want to install this package? (y/n): ";
//         std::getline(std::cin, input);
//     } else {
//         input = 'y';
//     }
//     if (!input.empty() && tolower(input[0]) != 'y') {
//         std::cout << "Installation canceled" << std::endl;
//         remove_all(temp_path);
//         return false;
//     }
//     //std::cout << "second pass: extract remaining" << std::endl;
//     std::vector<std::string> installed_files;
//     if (!Utilities::extractRemainingFiles(package_path,  metadata_files, installed_files)) {
//         remove_all(temp_path);
//         return false;
//     }
//
//     if (std::filesystem::path install_script = root / "install.anemonix"; exists(install_script)) {
//         if (system(install_script.string().c_str()) != 0) {
//             throw std::runtime_error("install.anemonix script failed");
//         }
//     }
//
//     // Insert package into DB
//     if(!Database::insertPkg(name, version, arch, deps_str, description)){
//         throw std::runtime_error("failed to insert pkg into the database!");
//     }
//
//     // Store installed_files in package database or process further
//     std::cout << "Installed " << installed_files.size() << " files\n";
//     Database::writePkgFilesBatch(name, installed_files);
//
//     // Insert provided items into the databases
//     for (const auto& [prov_name, prov_version] : provided_items) {
//         if (std::string desc = "provided by: " + name; !Database::insertPkg(prov_name, prov_version, arch, "", desc)) {
//             std::cerr << RED << "Failed to insert provided package: " << prov_name << RESET << "\n";
//         } else {
//             std::cout << GREEN << "-> Provided package " << prov_name << "=" << prov_version << " registered successfully!\n" << RESET;
//         }
//     }
//     // Mark package as broken if dependencies are missing
//     if (!missing_deps.empty()) {
//         Database::markAsBroken(name, deps_str);
//     }
//
//     std::cout << GREEN << ":) Successfully installed " << name << " v" << version << "\n" << RESET;
//     try{
//         remove_all(temp_path);
//     } catch(std::exception& e){
//         std::cout << "failed to remove package root. " << e.what() << std::endl;
//     }
//     remove_all(temp_path);
//     return true;
// }

// bool Anemo::install(const std::string& filename, bool force, bool reinstall) {
//
//     const std::string& package_path = filename;
//
//     std::vector<std::pair<std::string, std::string>> provided_items;
//     // Create temporary directory
//     std::string temp_dir = "/tmp/anemoXXXXXX";
//     std::filesystem::path temp_path(temp_dir);
//     if (!mkdtemp(temp_dir.data())) {
//         std::cerr << "Failed to create temporary directory" << std::endl;
//         return false;
//     }
//     std::string temp_dir_str(temp_dir);
//
//     // First pass: Extract metadata and scripts
//     std::unordered_set<std::string> metadata_files;
//     if (!Utilities::extractMetadataAndScripts(package_path, temp_dir_str, metadata_files, filename)) {
//         remove_all(temp_path);
//         return false;
//     }
//     // Verify metadata exists
//     std::string metadata_path = temp_dir_str + "/";
//     std::filesystem::path root(metadata_path);
//     if (access((root / "anemonix.yaml").c_str(), F_OK) != 0) {
//         std::cerr << "Missing required metadata file (anemonix.yaml)" << std::endl;
//         remove_all(temp_path);
//         return false;
//     }
//
//     Package pkg = Utilities::parseMetadata(root / "anemonix.yaml");
//
//     std::string deps_str = pkg.deps.empty() ? "" : fmt::format("{}", fmt::join(pkg.deps, ","));
//     std::cout << deps_str << std::endl;
//
//     std::string installed_version = Database::getPkgVersion(pkg.name);
//
//         char choice;
//         std::cin >> choice;
//         if (choice != 'y') {
//             std::cout << "Installation aborted.\n";
//             //remove_all(temp_path);
//             return false;
//         }
//
//         // Remove old package before upgrade/downgrade/reinstall
//         //Anemo::remove(pkg.name, force, true);   <-removal if user wants to update/downgrade
//
//
//     // Check for missing dependencies
//     std::vector<std::string> dependencies;
//     dependencies.reserve(pkg.deps.size());
// for (const auto& item : pkg.deps) {
//         dependencies.emplace_back(item->name);
//     }
//     std::vector<std::string> missing_deps = Database::checkDependencies(dependencies);
//
//     if (!force && !missing_deps.empty() && AConf::BSTRAP_PATH.empty()) {
//         std::cout << RED << "\n! Installation Failed: Missing Dependencies !\n" << RESET;
//         for (const auto& dep : missing_deps) {
//             std::cout << "-> " << YELLOW << dep << RESET << " is not installed.\n";
//         }
//         std::cout << "Use --force to install anyway.\n";
//         return false;
//     }
//     if (!missing_deps.empty()) {
//         std::cout << YELLOW << "Warning: Proceeding with installation despite missing dependencies.\n" << RESET;
//     }
//
//
//     //////////// ----CHECK FOR PACKAGES VERSIONS-----
//
//     // User confirmation
//     std::string input;
//     if (AConf::BSTRAP_PATH.empty()) {
//         std::cout << "Do you want to install this package? (y/n): ";
//         std::getline(std::cin, input);
//     } else {
//         input = 'y';
//     }
//     if (!input.empty() && tolower(input[0]) != 'y') {
//         std::cout << "Installation canceled" << std::endl;
//         //remove_all(temp_path);
//         return false;
//     }
//
//     //std::cout << "second pass: extract remaining" << std::endl;
//     std::vector<std::string> installed_files;
//     // if (!Utilities::extractRemainingFiles(package_path,  metadata_files, installed_files)) {
//     //     remove_all(temp_path);
//     //     return false;
//     // }
//
//     // assume successfull extraction
//
//     // assume succeffull install script execution
//     // if (std::filesystem::path install_script = root / "install.anemonix"; exists(install_script)) {
//     //     if (system(install_script.string().c_str()) != 0) {
//     //         throw std::runtime_error("install.anemonix script failed");
//     //     }
//     // }
//
//     // Insert package into DB
//     // if(!Database::insertPkg(pkg.name, pkg.version, pkg.arch, deps_str, pkg.description)){
//     //     throw std::runtime_error("failed to insert pkg into the database!");
//     // }
//
//     // assume insertion
//
//     std::cout << BOLD << GREEN << "[anemo :: debug :: database]    " << "inserted package " << pkg.name << "" << RESET << std::endl;
//
//     // Store installed_files in package database or process further
//     //std::cout << "Installed " << installed_files.size() << " files\n";
//     //Database::writePkgFilesBatch(pkg.name, installed_files);
//
//     // assume successfull insertion
//     for (const auto& [prov_name, prov_version] : provided_items) {
//         std::cout << BOLD << GREEN << "[anemo :: debug :: database]    " << "-> provided package " << prov_name << "=" << prov_version << " registered.\n" << RESET;
//     }
//
//
//     // Insert provided items into the databases
//     // for (const auto& [prov_name, prov_version] : provided_items) {
//     //     if (std::string desc = "provided by: " + pkg.name; !Database::insertPkg(prov_name, prov_version, pkg.arch, "", desc)) {
//     //         std::cerr << RED << "Failed to insert provided package: " << prov_name << RESET << "\n";
//     //     } else {
//     //         std::cout << GREEN << "-> Provided package " << prov_name << "=" << prov_version << " registered successfully!\n" << RESET;
//     //     }
//     // }
//
//     // Mark package as broken if dependencies are missing
//     if (!missing_deps.empty()) {
//         //Database::markAsBroken(pkg.name, deps_str);
//         std::cout << BOLD << YELLOW << "[anemo :: debug :: database]    " << "marked as broken " << pkg.name << "" << RESET << std::endl;
//     }
//
//     std::cout << GREEN << ":) Successfully installed " << pkg.name << " v" << pkg.version << "\n" << RESET;
//     // try{
//     //     remove_all(temp_path);
//     // } catch(std::exception& e){
//     //     std::cout << "failed to remove package root. " << e.what() << std::endl;
//     // }
//     //remove_all(temp_path);
//     return true;
// }

bool Anemo::install(const std::string& filename, bool force, bool reinstall) {
    const std::string& package_path = filename;

    geteuid() == 0;

    std::vector<std::pair<std::string, std::string>> provided_items;
    std::string temp_dir = "/tmp/anemoXXXXXX";
    std::filesystem::path temp_path(temp_dir);
    if (!mkdtemp(temp_dir.data())) {
        std::cerr << RED << "[ERROR] Failed to create temporary directory" << RESET << std::endl;
        return false;
    }
    std::string temp_dir_str(temp_dir);

    std::unordered_set<std::string> metadata_files;
    if (!Utilities::extractMetadataAndScripts(package_path, temp_dir_str, metadata_files, filename)) {
        remove_all(temp_path);
        return false;
    }

    std::string metadata_path = temp_dir_str + "/";
    std::filesystem::path root(metadata_path);
    if (access((root / "anemonix.yaml").c_str(), F_OK) != 0) {
        std::cerr << RED << "[ERROR] Missing required metadata file (anemonix.yaml)" << RESET << std::endl;
        remove_all(temp_path);
        return false;
    }

    Package pkg = Utilities::parseMetadata(root / "anemonix.yaml");
    pkg.print();

    std::vector<std::string> missing_deps;
    std::vector<std::string> conflicting_packages;

    for (const auto& dep : pkg.deps) {
        if (!AnemoDatabase.packageExists(dep)) {
            missing_deps.push_back(dep);
        }
    }

    for (const auto& conflict : pkg.conflicts) {
        if (AnemoDatabase.packageExists(conflict)) {
            conflicting_packages.push_back(conflict);
        }
    }

    if (!missing_deps.empty() || !conflicting_packages.empty()) {
        std::cout << BOLD << RED << "\n[ERROR] Cannot install " << pkg.name << " due to the following issues:" << RESET << std::endl;

        if (!missing_deps.empty()) {
            std::cout << BOLD << YELLOW << "\nMissing Dependencies:" << RESET << std::endl;
            for (const auto& dep : missing_deps) {
                std::cout << "  " << RED << "✘ " << dep << RESET << std::endl;
            }
        }

        if (!conflicting_packages.empty()) {
            std::cout << BOLD << MAGENTA << "\nConflicting Packages:" << RESET << std::endl;
            for (const auto& conflict : conflicting_packages) {
                std::cout << "  " << MAGENTA << "⚠ " << conflict << RESET << std::endl;
            }
        }

        std::cout << "\n";
        remove_all(temp_path);
        return false;
    }

    std::cout << BOLD << GREEN << "\n[INFO] All checks passed. Proceeding with installation..." << RESET << std::endl;
    AnemoDatabase.insertPackage(pkg);

    return true;
}

