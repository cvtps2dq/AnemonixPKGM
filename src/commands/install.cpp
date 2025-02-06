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
#include "defines.h"
#include <fmt/ranges.h>
#include <sys/xattr.h>

#include <sys/acl.h>

#include <sys/stat.h>

void preserveOwnership(const std::filesystem::path& source, const std::filesystem::path& destination) {
    struct stat file_stat;
    if (stat(source.c_str(), &file_stat) == 0) {
        chown(destination.c_str(), file_stat.st_uid, file_stat.st_gid);
    }
}

void preserveACLs(const std::filesystem::path& source, const std::filesystem::path& destination) {
    acl_t acl = acl_get_file(source.c_str(), ACL_TYPE_ACCESS);
    if (acl) {
        acl_set_file(destination.c_str(), ACL_TYPE_ACCESS, acl);
        acl_free(acl);
    }
}

void preserveExtendedAttributes(const std::filesystem::path& source, const std::filesystem::path& destination) {
    char attr_list[1024];
    ssize_t list_size = listxattr(source.c_str(), attr_list, sizeof(attr_list));

    if (list_size > 0) {
        for (ssize_t i = 0; i < list_size; i += strlen(&attr_list[i]) + 1) {
            char value[1024];
            ssize_t value_size = getxattr(source.c_str(), &attr_list[i], value, sizeof(value));
            if (value_size > 0) {
                setxattr(destination.c_str(), &attr_list[i], value, value_size, 0);
            }
        }
    }
}

void copyFileWithMetadata(const std::filesystem::path& source, const std::filesystem::path& destination) {
    try {
        // Extract the parent directory
        std::filesystem::path parent_dir = destination.parent_path();

        // Ensure the parent directory exists
        if (!parent_dir.empty() && !std::filesystem::exists(parent_dir)) {
            //std::filesystem::create_directories(parent_dir);
        }

        // Copy file while preserving symlinks
        std::filesystem::copy(source, destination, std::filesystem::copy_options::recursive |
            std::filesystem::copy_options::update_existing |
            std::filesystem::copy_options::copy_symlinks);

        // Preserve metadata
        preserveOwnership(source, destination);
        preserveACLs(source, destination);
        preserveExtendedAttributes(source, destination);

    } catch (const std::exception& e) {
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

bool installPkg(const std::filesystem::path &package_root, bool force, bool reinstall) {
    try {
        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        const auto name = config["name"].as<std::string>();
        const auto version = config["version"].as<std::string>();
        const auto arch = config["arch"].as<std::string>();

        std::vector<std::string> dependencies;
        if (config["deps"]) {
            for (const auto& dep : config["deps"]) {
                dependencies.push_back(dep.as<std::string>());
            }
        }
        std::string deps_str = dependencies.empty() ? "" : fmt::format("{}", fmt::join(dependencies, ","));

        
        // Check if package is already installed

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
                remove_all(package_root);
                return true;
            }

            char choice;
            std::cin >> choice;
            if (choice != 'y') {
                std::cout << "Installation aborted.\n";
                remove_all(package_root);
                return false;
            }

            // Remove old package before upgrade/downgrade/reinstall
            Anemo::remove(name, force, true);
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

        // Confirm installation
        char choice;
        if (AConf::BSTRAP_PATH.empty()) {
            std::cout << "Do you want to install this package? (y/n): ";
            std::cin >> choice;
        } else {
            choice = 'y';
        }

        if (choice == 'n') {
            std::cerr << "User aborted package installation\n";
            remove_all(package_root);
            return false;
        }

        // Run build script
        std::filesystem::path package_dir = package_root / "package";
        for (const auto& file : std::filesystem::directory_iterator(package_dir)) {
            std::filesystem::path target_path = "/" / file.path().lexically_relative(package_dir);
            std::filesystem::path full_target_path = AConf::BSTRAP_PATH + target_path.string();

            try {
                // Insert moved file path into database
                Database::writePkgFilesRecord(name, target_path.string());
                copyFileWithMetadata(file, full_target_path);
                //rename(file, full_target_path);

            } catch (const std::exception& e) {
                std::cerr << "Error moving " << file.path() << " -> " << full_target_path << ": " << e.what() << std::endl;
            }
        }

        if (std::filesystem::path install_script = package_root / "install.anemonix"; exists(install_script)) {
            if (system(install_script.string().c_str()) != 0) {
                throw std::runtime_error("install.anemonix script failed");
            }
        }

        // Insert package into DB
        if(!Database::insertPkg(name, version, arch, deps_str)){
            throw::std::runtime_error("failed to insert pkg into the database!");
        }

        
        // Mark package as broken if dependencies are missing
        if (!missing_deps.empty()) {
            Database::markAsBroken(name, deps_str);
        }


        std::cout << GREEN << ":) Successfully installed " << name << " v" << version << "\n" << RESET;
        try{
            remove_all(package_root);
        } catch(std::exception e){
            std::cout << "failed to remove package root. " << e.what() << std::endl;
        }
        
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Installation failed: " << e.what() << "\n";
        return false;
    }
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

    const std::string& package_file = arguments[0];  // First non-flag argument is the package file

    // Create temporary extraction directory
    std::filesystem::path temp_dir = AConf::ANEMO_ROOT + "/builds/tmp_" + std::to_string(getpid());
    if (!create_directory(temp_dir)) {
        std::cerr << RED << "err: failed to create temp directory :( \n" << RESET;
        return false;
    }

    // Step 1: Untar package
    if (!Utilities::untarPKG(package_file, temp_dir.string())) {
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
    if (!installPkg(target_dir, force, reinstall)) {
        std::cout << DSTRING << RED << "install failed :(" << std::endl << RESET;
        std::cout << DSTRING << RED << "removing package dir: " << target_dir << std::endl << RESET;
        remove_all(target_dir);
        return false;
    }

    return true;
}

