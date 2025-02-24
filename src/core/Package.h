//
// Created by cv2 on 24.02.2025.
//

#ifndef PACKAGE_H
#define PACKAGE_H

#include <iostream>
#include <vector>
#include <string>
#include <fmt/base.h>

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

    std::vector<std::shared_ptr<Package>> provides;
    std::vector<std::shared_ptr<Package>> deps;
    std::vector<std::shared_ptr<Package>> conflicts;
    std::vector<std::shared_ptr<Package>> replaces;

    // Constructor with default empty vectors
    Package(std::string name, std::string version, std::string author, std::string description, std::string arch,
            std::vector<std::shared_ptr<Package>> provides = {},
            std::vector<std::shared_ptr<Package>> deps = {},
            std::vector<std::shared_ptr<Package>> conflicts = {},
            std::vector<std::shared_ptr<Package>> replaces = {})
        : name(std::move(name)), version(std::move(version)), author(std::move(author)),
          description(std::move(description)), arch(std::move(arch)),
          provides(std::move(provides)), deps(std::move(deps)),
          conflicts(std::move(conflicts)), replaces(std::move(replaces)) {}

    // Destructor
    ~Package() {
        provides.clear();
        deps.clear();
        conflicts.clear();
        replaces.clear();
    }

    static void printTree(const std::vector<std::shared_ptr<Package>>& packages, const std::string& title, const std::string& color, const std::string& icon, const std::string& prefix = "  ") {
        if (packages.empty()) return;

        std::cout << BOLD << color << prefix << icon << " " << title << RESET << "\n";
        for (size_t i = 0; i < packages.size(); ++i) {
            const bool isLast = i == packages.size() - 1;
            std::cout << prefix << (isLast ? " ╰── " : " ├── ") << GREEN << " " << packages[i]->name
                      << RESET << " " << GRAY << "(" << packages[i]->version << ")" << RESET << "\n";
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

namespace fmt {
    template <>
    struct formatter<std::shared_ptr<Package>> {
        constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

        template <typename FormatContext>
        auto format(const std::shared_ptr<Package>& pkg, FormatContext& ctx) const {  // ← Marked as `const`
            return fmt::format_to(ctx.out(), "{} ({})", pkg->name, pkg->version);
        }
    };
} // namespace fmt

#endif // PACKAGE_H

