//
// Created by cv2 on 2/24/25.
//

#include "Utilities.h"
#include "Database.h"
#include <regex>

bool Utilities::satisfiesDependency(const std::string& dep, const Package& pkg) {
    // Extract package name and version constraint
    std::regex depRegex(R"(([^<>=]+)([<>=]*)(.+)?)");
    std::smatch match;

    if (!std::regex_match(dep, match, depRegex)) {
        std::cerr << "Error: Invalid dependency format: " << dep << std::endl;
        return false;
    }

    std::string depName = match[1].str();
    std::string constraint = match[2].str();
    std::string requiredVersion = match[3].str();

    // Check if package exists
    if (!AnemoDatabase.packageExists(depName)) {
        return false;
    }

    // Fetch installed version
    std::string installedVersion = AnemoDatabase.getPackage(depName).value().version;

    // If no constraint, any version satisfies
    if (constraint.empty()) {
        return true;
    }

    // Compare versions
    int cmp = Utilities::compareVersions(installedVersion, requiredVersion);

    if (constraint == "=") return cmp == 0;
    if (constraint == ">") return cmp > 0;
    if (constraint == "<") return cmp < 0;
    if (constraint == ">=") return cmp >= 0;
    if (constraint == "<=") return cmp <= 0;

    std::cerr << "Error: Unknown constraint: " << constraint << std::endl;
    return false;
}