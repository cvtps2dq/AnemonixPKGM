#include <Anemo.h>
#include <iostream>
#include <string>
#include <vector>
#include "config.h"

int main(const int argc, char* argv[]) {
    if (argc < 2) {
        Anemo::showHelp();
        return 1;
    }

    const std::string command = argv[1];
    bool force = false, reinstall = false, bootstrap = false;
    std::vector<std::string> arguments;

    // Parse arguments and flags
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--force") force = true;
        else if (arg == "--reinstall") reinstall = true;
        else if (arg == "--bootstrap") bootstrap = true;
        else arguments.push_back(arg);
    }

    if (command == "init") {
        if (bootstrap && !arguments.empty()) {
            AConf::ANEMO_ROOT = arguments[0] + AConf::ANEMO_ROOT;
            AConf::BSTRAP_PATH = arguments[0];
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
        }
        Anemo::init();
    } else if (command == "help") {
        Anemo::showHelp();
    } else if (command == "install" && !arguments.empty()) {
        if (bootstrap && arguments.size() > 1) {
            AConf::ANEMO_ROOT = arguments[1] + AConf::ANEMO_ROOT;
            AConf::BSTRAP_PATH = arguments[1];
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
            arguments.erase(arguments.begin()); // Remove path from package list
        }
        for (const auto& pkg : arguments) {
            if (!Anemo::install({pkg}, force, reinstall)) {
                std::cerr << "Failed to install: " << pkg << "\n";
            }
        }
    } else if (command == "remove" && !arguments.empty()) {
        std::string bootstrap_path;
        if (bootstrap && arguments.size() > 1) {
            bootstrap_path = arguments[0];
            AConf::ANEMO_ROOT = bootstrap_path + AConf::ANEMO_ROOT;
            AConf::BSTRAP_PATH = bootstrap_path;
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
            arguments.erase(arguments.begin()); // Remove path from package list
        }
        for (const auto& pkg : arguments) {
            if (!Anemo::remove(pkg, force, bootstrap)) {
                std::cerr << "Failed to remove: " << pkg << "\n";
            }
        }
    } else if (command == "audit") {
        if (bootstrap && !arguments.empty()) {
            AConf::ANEMO_ROOT = arguments[0] + AConf::ANEMO_ROOT;
            AConf::BSTRAP_PATH = arguments[0];
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
        }
        Anemo::audit();
    } else if (command == "list") {
        if (bootstrap && !arguments.empty()) {
            AConf::ANEMO_ROOT = arguments[0] + AConf::ANEMO_ROOT;
            AConf::BSTRAP_PATH = arguments[0];
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
        }
        Anemo::list();
    } else {
        std::cerr << "Unknown command\n";
        Anemo::showHelp();
        return 1;
    }
    return 0;
}