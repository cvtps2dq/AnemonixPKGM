//
// Created by cv2 on 2/24/25.
//

#include "Utilities.h"

std::vector<std::string> Utilities::splitString(const std::string& str) {
    std::vector<std::string> result;
    std::istringstream iss(str);
    std::string token;

    while (std::getline(iss, token, ',')) {
        if (!token.empty()) result.push_back(token);
    }

    return result;
}