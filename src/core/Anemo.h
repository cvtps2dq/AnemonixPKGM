//
// Created by cv2 on 04.02.2025.
//

#ifndef ANEMO_H
#define ANEMO_H
#include <__filesystem/path.h>


class Anemo {
public:
    static void showHelp();
    static bool init();
    static bool install(const std::filesystem::path& package_root, bool force, bool reinstall);
    static bool installCmd(int argc, char* argv[], bool force, bool reinstall);
    static bool remove(const std::string& name, bool force, bool update);
    static bool audit();
    static bool update();
    static bool upgrade();
    static bool validate(const std::filesystem::path& package_root);
};



#endif //ANEMO_H
