#ifndef PACKAGE_H
#define PACKAGE_H

#include <iostream>
#include <vector>
#include <string>

// ANSI color codes
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define GRAY    "\033[90m"

class Package {
public:

    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::string arch;
    std::vector<std::string> provides;
    std::vector<std::string> deps;
    std::vector<std::string> conflicts;
    std::vector<std::string> replaces;
    bool is_protected;

    // Constructor
    Package(std::string name, std::string version, std::string author, std::string description, std::string arch,
            std::vector<std::string> provides = {}, std::vector<std::string> deps = {},
            std::vector<std::string> conflicts = {}, std::vector<std::string> replaces = {}, bool is_protected = false)
        : name(std::move(name)), version(std::move(version)), author(std::move(author)),
          description(std::move(description)), arch(std::move(arch)),
          provides(std::move(provides)), deps(std::move(deps)),
          conflicts(std::move(conflicts)), replaces(std::move(replaces)),
          is_protected(is_protected) {}

    static void printTree(const std::vector<std::string>& packages, const std::string& title, const std::string& color, const std::string& icon, const std::string& prefix = "  ") {
        if (packages.empty()) return;

        std::cout << BOLD << color << prefix << icon << " " << title << RESET << "\n";
        for (size_t i = 0; i < packages.size(); ++i) {
            const bool isLast = i == packages.size() - 1;
            std::cout << prefix << (isLast ? " ╰── " : " ├── ") << GREEN << " " << packages[i] << RESET << "\n";
        }
    }

    void print() const {
        std::cout << "\n";
        std::cout << BOLD << CYAN << "╭────────────────────────────────────────────────╮\n";
        std::cout << BOLD << CYAN << "│ " << RESET << BOLD << name << RESET << "\n";
        std::cout << BOLD << CYAN << "│ ├── " << RESET << BLUE << "Version: " << RESET << version << "\n";
        std::cout << BOLD << CYAN << "│ ├── " << RESET << MAGENTA << "Architecture: " << RESET << arch << "\n";
        std::cout << BOLD << CYAN << "│ ├── " << RESET << YELLOW << "Author: " << RESET << author << "\n";
        std::cout << BOLD << CYAN << "│ ╰── " << RESET << CYAN << "Description: " << RESET << description << "\n";
        std::cout << BOLD << CYAN << "╰────────────────────────────────────────────────╯" << RESET << "\n";

        printTree(provides, "Provides", GREEN, "[P]");
        printTree(deps, "Dependencies", YELLOW, "[D]");
        printTree(conflicts, "⚠ Conflicts", RED, "[C]");
        printTree(replaces, "Replaces", MAGENTA, "[R]");

        std::cout << "\n";
    }
};


#endif // PACKAGE_H