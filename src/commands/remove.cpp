//
// Created by cv2 on 04.02.2025.
//

#include <colors.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <Database.h>
#include <fstream>
#include "Anemo.h"
#include <sqlite3.h>
#include <unistd.h>
#include <Utilities.h>
#include <sys/stat.h>

bool Anemo::remove(const std::string& name, bool force, bool update, bool iKnowWhatToDo) {
    std::cout << "Removing package: " << name << std::endl;

    // 1. Check if package exists
    if (!AnemoDatabase.packageExists(name)) {
        std::cerr << "Error: Package " << name << " is not installed.\n";
        return false;
    }

    // 2. Check if package is protected
    if (AnemoDatabase.isProtected(name) && iKnowWhatToDo) {
        std::cout << std::endl;
        std::cout << ANSI_BOLD << RED << "╭────────────────────────────────────────────────╮" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "│   Cannot proceed! This package is protected.   |" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "╰────────────────────────────────────────────────╯" << RESET << std::endl;
        std::cout << std::endl;

        std::cout << BOLD << "This package is protected from any changes. "
                             "It is essential for reliable system operation." << std::endl;

        std::cout << RED BOLD << "If you really wish to do"
                                 " operations on this package - use --i-know-what-to-do flag." << RESET << std::endl;
        exit(EXIT_FAILURE);
    }
    auto pkg = AnemoDatabase.getPackage(name);
    // 3. Check for reverse dependencies
    std::vector<std::string> reverseDeps = AnemoDatabase.getReverseDependencies(name, pkg.value().version);
    if (!reverseDeps.empty()) {
        if (!force) {
            std::cout << std::endl;
            std::cout << ANSI_BOLD << RED << "╭────────────────────────────────────────────────╮" << RESET << std::endl;
            std::cout << ANSI_BOLD << RED << "│         Removal halted due to errors!          │" << RESET << std::endl;
            std::cout << ANSI_BOLD << RED << "╰────────────────────────────────────────────────╯" << RESET << std::endl;
            std::cout << std::endl;

            std::cerr << "Error: The following packages depend on " << name << ":\n";
            for (size_t i = 0; i < reverseDeps.size(); ++i) {
                std::cerr << (i + 1 == reverseDeps.size() ? "   ╰──  " : "   ├──  ")
                          << reverseDeps[i] << std::endl;
            }
            std::cerr << "Use --force to remove anyway and mark them as broken.\n";
            return false;
        } else {

            std::cout << "Proceed with removal? (y/n): ";
            char response;
            std::cin >> response;
            if (response != 'y' && response != 'Y') {
                std::cout << "Installation cancelled." << std::endl;
                return false;
            }

            std::cout << "Marking the following packages as broken due to forced removal:\n";
            for (size_t i = 0; i < reverseDeps.size(); ++i) {
                std::cout << (i + 1 == reverseDeps.size() ? "   ╰──  " : "   ├──  ")
                          << reverseDeps[i] << std::endl;
                AnemoDatabase.markPackageAsBroken(reverseDeps[i]);
            }
        }
    }

    std::cout << "Proceed with removal? (y/n): ";
    char response;
    std::cin >> response;
    if (response != 'y' && response != 'Y') {
        std::cout << "Installation cancelled." << std::endl;
        return false;
    }

    // 4. Execute preremove script if it exists
    std::string preRemoveScript = "/var/lib/anemonix/scripts/" + name + "/preremove.anemonix";
    if (std::filesystem::exists(preRemoveScript)) {
        std::cout << "Executing preremove script...\n";
        if (!Utilities::runScript(preRemoveScript)) {
            std::cerr << "Warning: preremove script failed!\n";
        }
    }

    // 5. Remove installed files
    std::vector<std::string> installedFiles = AnemoDatabase.getPackageFiles(name);
    for (const std::string& file : installedFiles) {
        if (std::filesystem::exists(file)) {
            std::filesystem::remove(file);
        }
    }

    // 6. Execute postremove script if it exists
    std::string postRemoveScript = "/var/lib/anemonix/scripts/" + name + "/postremove.anemonix";
    if (std::filesystem::exists(postRemoveScript)) {
        std::cout << "Executing postremove script...\n";
        if (!Utilities::runScript(postRemoveScript)) {
            std::cerr << "Warning: postremove script failed!\n";
        }
    }

    // 7. Remove package scripts
    try{std::filesystem::remove_all("/var/lib/anemonix/scripts/" + name);} catch (const std::exception& e) {
        std::cout << std::endl;
        std::cout << ANSI_BOLD << RED << "╭────────────────────────────────────────────────╮" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "│            Failed to remove scripts!           |" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "╰────────────────────────────────────────────────╯" << RESET << std::endl;
        std::cout << std::endl;
        exit(EXIT_FAILURE);
    }

    // 8. Remove package entry from database
    try{AnemoDatabase.removePackage(name);} catch (const std::exception& e) {
        std::cout << std::endl;
        std::cout << ANSI_BOLD << RED << "╭────────────────────────────────────────────────╮" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "│     Failed to remove package from database!    |" << RESET << std::endl;
        std::cout << ANSI_BOLD << RED << "╰────────────────────────────────────────────────╯" << RESET << std::endl;
        std::cout << std::endl;
        exit(EXIT_FAILURE);
    }

    std::cout << "Package " << name << " removed successfully!\n";
    return true;
}