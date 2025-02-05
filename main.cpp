#include <Anemo.h>
#include <iostream>
#include <string>
#include "config.h"

int main(const int argc, char* argv[]) {
    if (argc < 2) {
        Anemo::showHelp();
        return 1;
    }

    const std::string command = argv[1];
    bool force = false;
    bool reinstall = false;
    bool bootstrap = false;

    // Check for --force flag
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--force") {
            force = true;
            break;
        }
    }

    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--bootstrap") {
            bootstrap = true;
            break;
        }
    }

    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--reinstall") {
            reinstall = true;
            break;
        }
    }

    if (command == "init") {
        if (bootstrap) {
            AConf::ANEMO_ROOT = std::string(argv[3]).append(AConf::ANEMO_ROOT);
            AConf::BSTRAP_PATH = std::string(argv[3]);
            std::cout << argv[3] << std::endl;
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
            std::cout << AConf::DB_PATH << std::endl;
        }
        Anemo::init();
    } else if (command == "help") {
        Anemo::showHelp();
    } else if (command == "install") {
        if (bootstrap) {
            AConf::ANEMO_ROOT = std::string(argv[4]).append(AConf::ANEMO_ROOT);
            AConf::BSTRAP_PATH = std::string(argv[4]);
            std::cout << argv[4] << std::endl;
            AConf::DB_PATH = AConf::ANEMO_ROOT + "/installed.db";
            std::cout << AConf::DB_PATH << std::endl;
        }
        Anemo::install(argc, argv, force, reinstall);
    } else if (command == "remove") {
        Anemo::remove(argv[2], force, false);
    } else if (command == "audit") {
        Anemo::audit();
    }else {
        std::cerr << "Unknown command\n";
        Anemo::showHelp();
        return 1;
    }

    return 0;
}