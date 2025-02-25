//
// Created by cv2 on 04.02.2025.
//

#include <iostream>
#include <unistd.h>
#include <Utilities.h>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <vector>
#include <Database.h>
#include <Filesystem.h>
#include <fstream>
#include <sys/stat.h>
#include "colors.h"
#include "Anemo.h"

void storePackageScripts(const std::string& packageName, const std::filesystem::path& path) {
    std::filesystem::path scriptDir = "/var/lib/anemonix/scripts/" + packageName;

    // Create script directory if it doesn't exist
    std::filesystem::create_directories(scriptDir);

    // Move known scripts to /var/lib/anemonix/scripts/package_name/
    std::vector<std::string> scriptNames = {"preinstall.anemonix",
        "postinstall.anemonix", "preremove.anemonix", "postremove.anemonix"};

    for (const std::string& script : scriptNames) {
        std::string src = path / script;
        std::string dest = scriptDir / script;

        if (std::filesystem::exists(src)) {
            std::filesystem::copy(src, dest);
            chmod(dest.c_str(), 0755);  // Ensure script is executable
        }
    }
}

bool Anemo::install(const std::string& filename, const bool force, const bool reinstall, const bool iKnowWhatToDo) {
    const std::string& package_path = filename;

    std::string temp_dir = "/tmp/anemoXXXXXX";
    if (!mkdtemp(temp_dir.data())) {
        std::cerr << RED << "[ERROR] Failed to create temporary directory" << RESET << std::endl;
        return false;
    }
    const std::filesystem::path temp_path(temp_dir);

    // Extract package using system tar
    const std::string extractCmd = "tar -xf " + package_path + " -C " + temp_dir + " --same-owner --acls --xattrs";
    if (system(extractCmd.c_str()) != 0) {
        std::cerr << "Error: Failed to extract package" << std::endl;
        remove_all(temp_path);
        return false;
    }

    const std::filesystem::path metadata_path = temp_path / "anemonix.yaml";
    if (!exists(metadata_path)) {
        std::cerr << RED << "[ERROR] Missing required metadata file (anemonix.yaml)" << RESET << std::endl;
        remove_all(temp_path);
        return false;
    }

    Package pkg = Utilities::parseMetadata(metadata_path);

    if (AnemoDatabase.packageExists(pkg.name) && !reinstall) {
        std::cout << "Package already installed." << std::endl;
        remove_all(temp_path);
        exit(0);
    }

    if (AnemoDatabase.isProtected(pkg.name)) {
        if (!iKnowWhatToDo) {
            std::cout << std::endl;
            std::cout << ANSI_BOLD << RED << "╭────────────────────────────────────────────────╮" << RESET << std::endl;
            std::cout << ANSI_BOLD << RED << "│   Cannot proceed! This package is protected.   |" << RESET << std::endl;
            std::cout << ANSI_BOLD << RED << "╰────────────────────────────────────────────────╯" << RESET << std::endl;
            std::cout << std::endl;

            std::cout << BOLD << "This package is protected from any changes. "
                                 "It is essential for reliable system operation." << std::endl;

            std::cout << RED BOLD << "If you really wish to do"
                                     " operations on this package - use --i-know-what-to-do flag." << RESET << std::endl;
        }

        remove_all(temp_path);
        exit(0);
    }

    // Step 1: Check dependencies
    std::vector<std::string> missing_deps;
    for (const std::string& dep : pkg.deps) {
        if (!Utilities::satisfiesDependency(dep, pkg)) {
            missing_deps.push_back(dep);
        }
    }

    // Step 2: Check conflicts
    std::vector<std::string> conflicting_pkgs;
    for (const std::string& conflict : pkg.conflicts) {
        if (AnemoDatabase.packageExists(conflict)) {
            conflicting_pkgs.push_back(conflict);
        }
    }

    // Step 3: Check reverse dependencies (affected packages)
    std::vector<std::string> affected_pkgs = AnemoDatabase.getReverseDependencies(pkg.name, pkg.version);
    std::vector<std::string> broken_pkgs;
    for (const std::string& affected : affected_pkgs) {
        Package affected_pkg = AnemoDatabase.getPackage(affected).value();
        for (const std::string& dep : affected_pkg.deps) {
            if (!Database::satisfiesDependency(dep, pkg.name, pkg.version)) {
                broken_pkgs.push_back(affected);
                break;
            }
        }
    }

    if (reinstall) {
        Anemo::remove(pkg.name, force, true, iKnowWhatToDo);
    }

    // Step 4: Display errors
    if ((!missing_deps.empty() || !conflicting_pkgs.empty() || !broken_pkgs.empty()) && AConf::BSTRAP_PATH.empty()) {
        if (!force) {
            std::cout << std::endl;
            std::cout << ANSI_BOLD << RED << "╭────────────────────────────────────────────────╮" << RESET << std::endl;
            std::cout << ANSI_BOLD << RED << "│      Installation halted due to errors!        |" << RESET << std::endl;
            std::cout << ANSI_BOLD << RED << "╰────────────────────────────────────────────────╯" << RESET << std::endl;
            std::cout << std::endl;
        }

        if (!broken_pkgs.empty() && !force) {
            std::cout << ANSI_BOLD << RED << "  [A] Affected Packages (May Break)" << RESET << std::endl;
            for (size_t i = 0; i < broken_pkgs.size(); ++i) {
                std::cout << (i + 1 == broken_pkgs.size() ? "   ╰──  " : "   ├──  ")
                          << broken_pkgs[i] << std::endl;
            }
        }

        if (!missing_deps.empty() && !force) {
            std::cout << ANSI_BOLD << ANSI_YELLOW << "  [M] Missing Dependencies" << RESET << std::endl;
            for (size_t i = 0; i < missing_deps.size(); ++i) {
                std::cout << (i + 1 == missing_deps.size() ? "   ╰──  " : "   ├──  ")
                          << missing_deps[i] << std::endl;
            }
        }

        if (!conflicting_pkgs.empty()) {
            std::cout << std::endl;
            std::cout << ANSI_BOLD << RED << "  [C] Conflicts" << RESET << std::endl;
            for (size_t i = 0; i < conflicting_pkgs.size(); ++i) {
                std::cout << (i + 1 == conflicting_pkgs.size() ? "   ╰──  " : "   ├──  ")
                          << conflicting_pkgs[i] << std::endl;
            }
        }

        if ((!conflicting_pkgs.empty() && force) || !force) {
            std::cout << std::endl;
            remove_all(temp_path);
            return false;
        }

    }

    // Confirm installation
    std::cout << "Proceed with installation? (y/n): ";
    char response;
    std::cin >> response;
    if (response != 'y' && response != 'Y') {
        std::cout << "Installation cancelled." << std::endl;
        return false;
    }

    // Run pre-install script if it exists
    std::filesystem::path installScript = temp_path / "preinstall.anemonix";
    if (access(installScript.c_str(), F_OK) == 0) {
        std::cout << "Running install script..." << std::endl;
        if (!Utilities::runScript(installScript)) {
            std::cerr << "Error: Package installation script failed!" << std::endl;
            return false;
        }
    }

    // Install files with rsync
    const std::string rsyncCmd = "rsync -a " + temp_dir + "/package/ " + AConf::BSTRAP_PATH + "/";
    if (system(rsyncCmd.c_str()) != 0) {
        remove_all(temp_path);
        std::cout << std::endl;
        std::cout << ANSI_BOLD << RED << "╭────────────────────────────────────────────────╮" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "│              Failed to copy files!             |" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "╰────────────────────────────────────────────────╯" << RESET << std::endl;
        std::cout << std::endl;
        exit(EXIT_FAILURE);
    }

    // Insert package into database
    try {
        if (force && (!missing_deps.empty() || !broken_pkgs.empty() || !conflicting_pkgs.empty())) {
            AnemoDatabase.insertPackage(pkg, true);
        } else AnemoDatabase.insertPackage(pkg, false);
    } catch ([[maybe_unused]] const std::exception& e) {
        remove_all(temp_path);
        std::cout << ANSI_BOLD << RED << "Failed to insert package into database!" << RESET << std::endl;
        exit(EXIT_FAILURE);
    }

    // insert provided packages
    try {
        AnemoDatabase.insertProvided(pkg);
    } catch ([[maybe_unused]] const std::exception& e) {
        remove_all(temp_path);
        std::cout << std::endl;
        std::cout << ANSI_BOLD << RED << "╭────────────────────────────────────────────────╮" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "│      Failed to register provided packages!     |" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "╰────────────────────────────────────────────────╯" << RESET << std::endl;
        std::cout << std::endl;
        exit(EXIT_FAILURE);
    }

    // Insert files into database
    try {
        std::vector<std::string> installed_files;
        for (const auto& file : std::filesystem::recursive_directory_iterator(temp_dir + "/package/")) {
            if (!file.is_directory()) {
                installed_files.push_back(file.path().string().substr(temp_dir.length() + 8));
            }
        }
        AnemoDatabase.insertFiles(pkg.name, installed_files);
    } catch ([[maybe_unused]] const std::exception& e) {
        remove_all(temp_path);
        std::cout << std::endl;
        std::cout << ANSI_BOLD << RED << "╭────────────────────────────────────────────────╮" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "│      Failed to insert files into database!     |" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "╰────────────────────────────────────────────────╯" << RESET << std::endl;
        std::cout << std::endl;
        exit(EXIT_FAILURE);
    }

    installScript = temp_path / "postinstall.anemonix";
    if (access(installScript.c_str(), F_OK) == 0) {
        std::cout << "Running install script..." << std::endl;
        if (!Utilities::runScript(installScript)) {
            std::cerr << "Error: Package installation script failed!" << std::endl;
            return false;
        }
    }

    // store scripts for later

    storePackageScripts(pkg.name, temp_path);

    std::cout << GREEN << "[ OK ] Package installed successfully: " << pkg.name << RESET << std::endl;
    return true;
}

