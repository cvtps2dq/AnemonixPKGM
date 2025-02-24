#include <Anemo.h>
#include <colors.h>
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
    bool force = false, reinstall = false, bootstrap = false, iKnowWhatToDo = false;
    std::string bootstrap_path;
    std::vector<std::string> arguments;

    // Parse arguments correctly
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--force") force = true;
        else if (arg == "--i-know-what-to-do") iKnowWhatToDo = true;
        else if (arg == "--reinstall") reinstall = true;
        else if (arg == "--bootstrap") {
            bootstrap = true;
            if (i + 1 < argc) { // Ensure there's another argument after --bootstrap
                bootstrap_path = argv[++i]; // Capture the path and skip next iteration
            } else {
                std::cerr << "err: --bootstrap flag requires a path argument\n";
                return 1;
            }
        } else {
            arguments.push_back(arg);
        }
    }

    // Handle bootstrap mode by updating paths
    if (bootstrap) {
        if (bootstrap_path.empty()) {
            std::cerr << "err: Missing bootstrap path\n";
            return 1;
        }
        if (bootstrap_path == "/") {
            std::cerr << "Bootstrapping to the root is just installing to the system." << std::endl;
            return 1;
        }
        AConf::ANEMO_ROOT = bootstrap_path + AConf::ANEMO_ROOT;
        AConf::BSTRAP_PATH = bootstrap_path;
        AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
    }

    if (command == "init") {
        Anemo::init();
    } else if (command == "help") {
        Anemo::showHelp();
    } else if (command == "install" && !arguments.empty()) {
        for (const auto& pkg : arguments) {
            if (!Anemo::install({pkg}, force, reinstall, iKnowWhatToDo)) {
                std::cerr << RED << "Failed to install: " << pkg << RESET << "\n";
            }
        }
    } else if (command == "remove" && !arguments.empty()) {
        for (const auto& pkg : arguments) {
            if (!Anemo::remove(pkg, force, bootstrap)) {
                std::cerr << "Failed to remove: " << pkg << "\n";
            }
        }
    } else if (command == "audit") {
        if (bootstrap) {
            if (bootstrap_path.empty()) {
                std::cerr << "err: Missing bootstrap path\n";
                return 1;
            }
            AConf::ANEMO_ROOT = bootstrap_path + AConf::ANEMO_ROOT;
            AConf::BSTRAP_PATH = bootstrap_path;
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
        }
        Anemo::audit();
    } else if (command == "list") {
        if (bootstrap) {
            if (bootstrap_path.empty()) {
                std::cerr << "err: Missing bootstrap path\n";
                return 1;
            }
            AConf::ANEMO_ROOT = bootstrap_path + AConf::ANEMO_ROOT;
            AConf::BSTRAP_PATH = bootstrap_path;
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
        }
        Anemo::list();
    } else if (command == "list-parse") {
        if (bootstrap) {
            if (bootstrap_path.empty()) {
                std::cerr << "err: Missing bootstrap path\n";
                return 1;
            }
            AConf::ANEMO_ROOT = bootstrap_path + AConf::ANEMO_ROOT;
            AConf::BSTRAP_PATH = bootstrap_path;
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
        }
        Anemo::listParse();
    } else if (command == "info") {
        Anemo::info(arguments[0]);
    }
    else if (command == "count") {
        if (bootstrap) {
            if (bootstrap_path.empty()) {
                std::cerr << "err: Missing bootstrap path\n";
                return 1;
            }
            AConf::ANEMO_ROOT = bootstrap_path + AConf::ANEMO_ROOT;
            AConf::BSTRAP_PATH = bootstrap_path;
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
        }
        Anemo::count();
    }else {
        std::cerr << "Unknown command\n";
        Anemo::showHelp();
        return 1;
    }

    return 0;
}