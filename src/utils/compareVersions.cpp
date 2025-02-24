//
// Created by cv2 on 2/24/25.
//

#include "Utilities.h"

int Utilities::compareVersions(const std::string& v1, const std::string& v2) {
    std::vector<int> ver1, ver2;

    auto parseVersion = [](const std::string& version, std::vector<int>& parsed) {
        std::stringstream ss(version);
        std::string segment;
        while (std::getline(ss, segment, '.')) {
            parsed.push_back(std::stoi(segment));
        }
    };

    parseVersion(v1, ver1);
    parseVersion(v2, ver2);

    // Normalize versions to have the same length by appending zeros
    while (ver1.size() < ver2.size()) ver1.push_back(0);
    while (ver2.size() < ver1.size()) ver2.push_back(0);

    // Compare each part (major, minor, patch, etc.)
    for (size_t i = 0; i < ver1.size(); ++i) {
        if (ver1[i] < ver2[i]) return -1; // v1 < v2
        if (ver1[i] > ver2[i]) return 1;  // v1 > v2
    }

    return 0; // Versions are equal
}