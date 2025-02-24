//
// Created by cv2 on 2/24/25.
//

#include "Database.h"
#include "Utilities.h"
#include <regex>

bool parseDependency(const std::string& dependency, std::string& name, std::string& op, std::string& version) {
    std::regex depRegex(R"(^([\w\-.]+)([<>=!]*)([\d\w\-.]*)$)");
    std::smatch match;

    if (!std::regex_match(dependency, match, depRegex)) {
        return false;  // Invalid format
    }

    name = match[1].str();
    op = match[2].str();
    version = match[3].str();

    // Default to "=" if no operator is provided
    if (op.empty() && !version.empty()) op = "=";

    return true;
}

int versionCompare(const std::string& v1, const std::string& v2) {
    std::istringstream ss1(v1), ss2(v2);
    int num1 = 0, num2 = 0;
    char dot;

    while (ss1 || ss2) {
        ss1 >> num1;
        ss2 >> num2;

        if (num1 < num2) return -1;
        if (num1 > num2) return 1;

        // Consume dots (.)
        ss1 >> dot;
        ss2 >> dot;
    }

    return 0;
}

bool compareVersions(const std::string& installed, const std::string& op, const std::string& required) {
    int cmp = versionCompare(installed, required);

    if (op == "=") return cmp == 0;
    if (op == "!=") return cmp != 0;
    if (op == ">") return cmp > 0;
    if (op == "<") return cmp < 0;
    if (op == ">=") return cmp >= 0;
    if (op == "<=") return cmp <= 0;

    return false;
}


bool Database::satisfiesDependency(const std::string& dep, const std::string& packageName, const std::string& packageVersion) {
    // Example dependency format: "yaml-cpp=0.8.0" or "sqlite3>=3.4.0"
    std::regex depRegex(R"(([^<>=]+)([<>=]*)([\d\.]+)?)");
    std::smatch match;

    if (std::regex_match(dep, match, depRegex)) {
        std::string depName = match[1].str();
        std::string op = match[2].str();
        std::string requiredVersion = match[3].str();

        if (depName != packageName) {
            return false;  // It's a different package
        }

        if (requiredVersion.empty()) {
            return true;  // No version constraint, any version is fine
        }

        return compareVersions(packageVersion, op, requiredVersion);
    }

    return false;
}
