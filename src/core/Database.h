//
// Created by cv2 on 04.02.2025.
//

#ifndef DATABASE_H
#define DATABASE_H

#include <vector>


class Database {
    public:
    static bool init();
    static std::vector<std::string> checkDependencies(const std::vector<std::string> deps, const std::string& name);
    static std::string getPkgVersion(const std::string& name);
    static bool writePkgFilesRecord(const std::string& name, const std::string& path);
    static bool insertPkg(const std::string& name, const std::string& version, const std::string& arch, const std::string& deps);
    static bool markAsBroken(const std::string& name, const std::string& missingDeps);
};



#endif //DATABASE_H
