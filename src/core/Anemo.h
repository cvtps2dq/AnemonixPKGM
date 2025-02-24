//
// Created by cv2 on 04.02.2025.
//

#ifndef ANEMO_H
#define ANEMO_H

#if defined(__APPLE__) && defined(__MACH__)
    #include <__filesystem/operations.h>
#elif defined (__linux__)
    #include <filesystem>
    #include <vector>
    #include <string>
#endif


class Anemo {
public:
    static void showHelp();
    static bool init();
    static bool install(const std::string &filename, bool force, bool reinstall, bool iKnowWhatToDo);
    static bool remove(const std::string& name, bool force, bool update, bool iKnowWhatToDo);
    static bool audit();
    static bool update();
    static bool upgrade();
    static bool validate(const std::filesystem::path& package_root);
    static int count();
    static void list();
    static void info(const std::string &query);

    static void listParse();
};

#endif