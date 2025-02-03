//
// Created by cv2 on 03.02.2025.
//

#include "../include/cli/cli.h"
#include <iostream>

namespace anemonix {

    Cli::Cli() {
        // Register commands
        register_command("install", std::make_unique<InstallCommand>());
        register_command("build", std::make_unique<BuildCommand>());
        // Add other commands...
    }

    void Cli::register_command(const std::string& name, std::unique_ptr<CliCommand> cmd) {
        commands_[name] = std::move(cmd);
    }

    int Cli::execute(int argc, char** argv) {
        program_name_ = argv[0];

        if (argc < 2) {
            print_help();
            return 1;
        }

        std::string cmd_name = argv[1];
        auto it = commands_.find(cmd_name);

        if (it == commands_.end()) {
            std::cerr << "Error: Unknown command '" << cmd_name << "'\n";
            print_help();
            return 1;
        }

        return it->second->execute(argc - 1, argv + 1);
    }

    void Cli::print_help() const {
        std::cout << "Usage: " << program_name_ << " <command> [options]\n\n";
        std::cout << "Available commands:\n";

        for (const auto& [name, cmd] : commands_) {
            std::cout << "  " << name << "\t" << cmd->help() << "\n";
        }
    }

    // InstallCommand implementation
    int InstallCommand::execute(int argc, char** argv) {
        if (argc < 2) {
            std::cerr << "Error: Missing package name\n";
            return 1;
        }

        std::string pkg_name = argv[1];
        std::cout << "Installing package: " << pkg_name << "\n";
        // Implementation...
        return 0;
    }

    std::string InstallCommand::help() const {
        return "Install a package (from source or binary)";
    }

    // BuildCommand implementation
    int BuildCommand::execute(int argc, char** argv) {
        if (argc < 2) {
            std::cerr << "Error: Missing package path\n";
            return 1;
        }

        std::string pkg_path = argv[1];
        std::cout << "Building package: " << pkg_path << "\n";
        // Implementation...
        return 0;
    }

    std::string BuildCommand::help() const {
        return "Build a package from source";
    }

} // namespace anemonix