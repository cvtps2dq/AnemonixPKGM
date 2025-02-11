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

// void preserveOwnership(const std::filesystem::path& source, const std::filesystem::path& destination) {
//     struct stat file_stat;
//     if (stat(source.c_str(), &file_stat) == 0) {
//         chown(destination.c_str(), file_stat.st_uid, file_stat.st_gid);
//     }
// }

// void preserveACLs(const std::filesystem::path& source, const std::filesystem::path& destination) {
//     acl_t acl = acl_get_file(source.c_str(), ACL_TYPE_ACCESS);
//     if (acl) {
//         acl_set_file(destination.c_str(), ACL_TYPE_ACCESS, acl);
//         acl_free(acl);
//     }
// }

// void preserveExtendedAttributes(const std::filesystem::path& source, const std::filesystem::path& destination) {
//     char attr_list[1024];
//     ssize_t list_size = listxattr(source.c_str(), attr_list, sizeof(attr_list));
//
//     if (list_size > 0) {
//         for (ssize_t i = 0; i < list_size; i += strlen(&attr_list[i]) + 1) {
//             char value[1024];
//             ssize_t value_size = getxattr(source.c_str(), &attr_list[i], value, sizeof(value));
//             if (value_size > 0) {
//                 setxattr(destination.c_str(), &attr_list[i], value, value_size, 0);
//             }
//         }
//     }
// }

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

bool installPkg(const std::filesystem::path &package_root, bool force, bool reinstall) {
    try {
        std::string description = "none";
        YAML::Node config = YAML::LoadFile(package_root / "anemonix.yaml");
        const auto name = config["name"].as<std::string>();
        const auto version = config["version"].as<std::string>();
        const auto arch = config["arch"].as<std::string>();
        if (config["description"]) description = config["description"].as<std::string>();

        // Parse 'provides'
        std::vector<std::pair<std::string, std::string>> provided_items;
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
        constexpr char spin_chars[] = {'|', '/', '-', '\\'};
        int spin_index = 0;
        try {
            std::filesystem::copy(package_dir, AConf::BSTRAP_PATH + "/",
                std::filesystem::copy_options::recursive |
                    std::filesystem::copy_options::copy_symlinks |
                    std::filesystem::copy_options::update_existing);
        } catch ([[maybe_unused]] const std::exception &e) {
            std::cerr << YELLOW << "warn :: write skip, file exists" << RESET << "\n";
        }
        int ix = 0;
        std::vector<std::string> files_to_register;

        // Collect files first
        for (const auto& file : std::filesystem::recursive_directory_iterator(package_dir)) {
            std::filesystem::path target_path = file.path().lexically_relative(package_dir);
            std::filesystem::path full_target_path = AConf::BSTRAP_PATH + target_path.string();

            if (ix % 4 == 0) {
                std::cout << "\r[ " << spin_chars[spin_index] << " ] collecting files";
                spin_index = (spin_index + 1) % 4;
            }

            if (!is_directory(file)) {
                files_to_register.push_back(full_target_path);
            }
            ix++;
        }

        // Batch insert files into the database
        Database::writePkgFilesBatch(name, files_to_register);
        std::cout << std::endl;
        std::cout << "[ OK ] Copying done.";
        std::cout << std::endl;

        if (std::filesystem::path install_script = package_root / "install.anemonix"; exists(install_script)) {
            if (system(install_script.string().c_str()) != 0) {
                throw std::runtime_error("install.anemonix script failed");
            }
        }

        // Insert package into DB
        if(!Database::insertPkg(name, version, arch, deps_str, description)){
            throw std::runtime_error("failed to insert pkg into the database!");
        }

        // Insert provided items into the database
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

            remove_all(package_root);
        } catch(std::exception& e){
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

