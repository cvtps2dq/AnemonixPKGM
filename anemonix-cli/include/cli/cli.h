//
// Created by cv2 on 03.02.2025.
//

#ifndef CLI_H
#define CLI_H

// include/cli/cli.hpp
#pragma once
#include <string>
#include <vector>
#include <memory>

namespace anemonix {

    class CliCommand {
    public:
        virtual ~CliCommand() = default;
        virtual int execute(int argc, char** argv) = 0;
        virtual std::string help() const = 0;
    };

    class Cli {
    public:
        Cli();

        int execute(int argc, char** argv);
        void register_command(const std::string& name, std::unique_ptr<CliCommand> cmd);

    private:
        std::unordered_map<std::string, std::unique_ptr<CliCommand>> commands_;
        std::string program_name_;

        void print_help() const;
    };

    // Concrete command classes
    class InstallCommand : public CliCommand {
    public:
        int execute(int argc, char** argv) override;
        std::string help() const override;
    };

    class BuildCommand : public CliCommand {
    public:
        int execute(int argc, char** argv) override;
        std::string help() const override;
    };

} // namespace anemonix

#endif //CLI_H
