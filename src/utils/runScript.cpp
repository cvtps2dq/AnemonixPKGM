//
// Created by cv2 on 2/24/25.
//

#include <fstream>
#include <unistd.h>
#include <sys/stat.h>

#include "Utilities.h"

bool Utilities::runScript(const std::string& scriptPath) {
    if (scriptPath.empty()) {
        std::cerr << "Error: No script path provided!\n";
        return false;
    }

    if (access(scriptPath.c_str(), F_OK) != 0) {
        std::cerr << "Error: Script file not found: " << scriptPath << "\n";
        return false;
    }

    // Make sure script is executable
    chmod(scriptPath.c_str(), 0755);

    // Command to execute
    std::ostringstream cmd;
    cmd << "/bin/bash " << scriptPath << " > /tmp/anemo_script.log 2>&1";

    // Run script and capture exit code
    int exitCode = std::system(cmd.str().c_str());

    // Log script output
    std::ifstream logFile("/tmp/anemo_script.log");
    std::string line;
    while (std::getline(logFile, line)) {
        std::cout << "[SCRIPT] " << line << std::endl;
    }
    logFile.close();

    // Check if script execution was successful
    if (exitCode != 0) {
        std::cerr << "Error: Script failed with exit code " << exitCode << "\n";
        return false;
    }

    return true;
}