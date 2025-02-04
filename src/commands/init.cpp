//
// Created by cv2 on 04.02.2025.
//

#include <Database.h>
#include <iostream>
#include <Utilities.h>

#include "Anemo.h"

bool Anemo::init() {
    std::cout << "Initializing Anemonix...\n";
    if (!Utilities::initFolders() || !Database::init()) {
        std::cerr << "Failed to initialize Anemonix." << std::endl;
        return false;
    }
    std::cout << "Anemonix initialized successfully.\n";
    return true;
}
