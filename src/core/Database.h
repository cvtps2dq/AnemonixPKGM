//
// Created by cv2 on 04.02.2025.
//

#ifndef DATABASE_H
#define DATABASE_H

#include <vector>
#include <string>


class Database {
    public:
    static bool init();
    static std::vector<std::string> checkDependencies(const std::vector<std::string> &deps);
    static std::string getPkgVersion(const std::string& name);
    static bool writePkgFilesRecord(const std::string& name, const std::string& target_path);

    static std::tuple<std::string, std::string> fetchNameAndVersion(const std::string &name);

    static std::vector<std::string> fetchFiles(const std::string &name);

    static bool markAffected(const std::vector<std::string>& dependent_packages);

    static bool removePkg(const std::string &name);

    static std::vector<std::tuple<std::string, std::string, std::string>> fetchAllPkgs();

    static std::vector<std::pair<std::string, std::vector<std::string>>> fetchAllBroken();

    static bool auditPkgs(const std::vector<std::pair<std::string, std::vector<std::string>>> &broken_packages);

    static bool insertPkg(const std::string& name, const std::string& version,
                          const std::string& arch, const std::string& deps_str);
    static bool markAsBroken(const std::string& name, const std::string& deps_str);
    static std::vector<std::string> checkPkgReliance(const std::string& name);
};



#endif //DATABASE_H
