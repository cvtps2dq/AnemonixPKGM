#include <Anemo.h>
#include <iostream>
#include <string>

int main(const int argc, char* argv[]) {
    if (argc < 2) {
        Anemo::showHelp();
        return 1;
    }
    if (const std::string command = argv[1]; command == "init") {
        Anemo::init();
    } else if (command == "help") {
        Anemo::showHelp();
    } else if (command == "install") {
        Anemo::installCmd(argc, argv);
    } else if (command == "remove") {
        Anemo::remove(argv[2]);
    }
    else {
        std::cerr << "Unknown command\n";
        Anemo::showHelp();
        return 1;
    }

    return 0;
}