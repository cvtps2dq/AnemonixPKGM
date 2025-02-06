//
// Created by cv2 on 04.02.2025.
//

#include <colors.h>
#include <iostream>

#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/operations.h>
#elif defined (__linux__)
    #include <filesystem>
    #include <vector>
#endif

#include <Database.h>

#include "Anemo.h"
#include "config.h"

bool Anemo::remove(const std::string &name, const bool force, const bool update) {
    try {
        const std::vector<std::string> dependent_packages = Database::checkPkgReliance(name);

        if (!dependent_packages.empty() && !force && !update) {
            std::cout << RED << "\n! Cannot Remove: " << name << " !\n" << RESET;
            std::cout << "----------------------------------------\n";
            std::cout << "It is required by: " << YELLOW << "\n";
            for (const auto& dep : dependent_packages) {
                std::cout << dep << std::endl;
            }
            std::cout << "! Aborting removal.\n";
            return false;
        }

        // Fetch package metadata
        const std::string version = std::get<0>(Database::fetchNameAndVersion(name));
        const std::string arch = std::get<1>(Database::fetchNameAndVersion(name));

        if (const std::string desc = Database::fetchDescription(name); desc.contains("provided by:")) {
            std::cout << desc << std::endl;
            std::cout << RED << "\n! Cannot Remove: " << name << " !\n" << RESET;
            std::cout << "----------------------------------------\n";
            std::cout << "this package is provided by: " << YELLOW << "\n";
            std::cout << "-> " << desc.substr(13, std::string::npos) << YELLOW << "\n" << RESET;
            std::cout << CYAN <<"remove that package to remove this one automatically" << "\n" << RESET;
            std::cout << "! Aborting removal.\n";
        }

        // Fetch file list
        const std::vector<std::string> files = Database::fetchFiles(name);

        if (files.empty()) {
            std::cerr << "No files recorded for package '" << name << "'.\n";
            return false;
        }

        // Fetch provided packages
        const std::vector<std::string> provided_packages = Database::fetchProvidedPackages(name);



        // Print metadata & files
        std::cout << "\n! Package Removal Confirmation !\n";
        std::cout << "------------------------------------\n";
        std::cout << "Name    : " << name << "\n";
        std::cout << "Version : " << version << "\n";
        std::cout << "Arch    : " << arch << "\n";
        std::cout << "------------------------------------\n";
        std::cout << "Installed Files:\n";
        for (const auto& file : files) {
            std::cout << "   -> " << file << "\n";
        }
        std::cout << "------------------------------------\n";
        std::cout << "Provided Packages:\n";
        for (const auto& prov : provided_packages) {
            std::cout << "   -> " << prov << "\n";
        }
        std::cout << "------------------------------------\n";
        std::cout << "Proceed with removal? (y/n): ";

        char choice;
        std::cin >> choice;
        if (choice == 'n') {
            std::cerr << "User aborted package removal.\n";
            return false;
        }
        if (choice != 'y') {
            std::cerr << "Invalid choice.\n";
            return false;
        }

        // Remove files
        for (const auto& file : files) {
            std::filesystem::remove(file);
        }

        // If force flag is used, move affected packages to broken_packages table
        if (force && !dependent_packages.empty() && !update)
            Database::markAffected(dependent_packages);

        // Remove database entries for the main package
        Database::removePkg(name);

        // Remove provided packages
        std::string prov_name;
        for (const auto& prov : provided_packages) {
            size_t pos = prov.find('=');
            if (pos != std::string::npos) {
                prov_name = prov.substr(0, pos);
            }
            Database::removePkg(prov_name);
            std::cout << "[OK] Removed provided package: " << prov_name << "\n";
        }

        std::cout << "[OK] Successfully removed " << name << "\n";
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[FL] Removal failed: " << e.what() << " :(\n";
        return false;
    }
}