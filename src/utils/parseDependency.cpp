//
// Created by cv2 on 2/24/25.
//

#include "Utilities.h"

std::shared_ptr<Package> Utilities::parseDependency(const std::string& dep_entry) {
    std::string name, version, constraint_type;

    // Check for >= constraint
    if (dep_entry.find(">=") != std::string::npos) {
        size_t pos = dep_entry.find(">=");
        name = dep_entry.substr(0, pos);
        version = "," +dep_entry.substr(pos + 2);
        constraint_type = "greater";  // Indicates >= constraint
    }
    // Check for <= constraint
    else if (dep_entry.find("<=") != std::string::npos) {
        size_t pos = dep_entry.find("<=");
        name = dep_entry.substr(0, pos);
        version = "," + dep_entry.substr(pos + 2);
        constraint_type = "less";  // Indicates <= constraint
    }
    // Check for strict =
    else if (dep_entry.find('=') != std::string::npos) {
        size_t pos = dep_entry.find('=');
        name = dep_entry.substr(0, pos);
        version = "," + dep_entry.substr(pos + 1);
        constraint_type = "strict";  // Indicates exact version match
    }
    // No version specified, assume latest
    else {
        name = dep_entry;
        version = "";
        constraint_type = "any";  // No specific version constraint
    }

    return std::make_shared<Package>(name, constraint_type + version, "", "", "");
}