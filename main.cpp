#include <Anemo.h>
#include <iostream>
#include <string>

int main(const int argc, char* argv[]) {
    if (argc < 2) {
        Anemo::showHelp();
        return 1;
    }

    const std::string command = argv[1];
    bool force = false;
    bool reinstall = false;

    // Check for --force flag
    for (int i = 2; i < argc; ++i) {
        if (std::string(argv[i]) == "--force") {
            force = true;
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
        Anemo::init();
    } else if (command == "help") {
        Anemo::showHelp();
    } else if (command == "install") {
        Anemo::installCmd(argc, argv, force, reinstall);
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