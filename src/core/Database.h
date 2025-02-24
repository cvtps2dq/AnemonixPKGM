//
// Created by cv2 on 04.02.2025.
//

#ifndef DATABASE_H
#define DATABASE_H

#include <config.h>
#include <queue>
#include <sqlite3.h>
#include <sstream>
#include <vector>
#include <string>
#include <fmt/ranges.h>

#include "Package.h"


class Database {
private:
    sqlite3 *db = {};
public:
    bool packageExists(const std::string &packageName);
    bool insertPackage(const Package &pkg, bool broken);
    std::optional<Package> getPackage(const std::string &packageName);
    bool init();

    bool isProtected(const std::string &packageName);

    bool insertFiles(const std::string &packageName, const std::vector<std::string> &files);

    std::vector<std::string> getReverseDependencies(const std::string &packageName, const std::string &packageVersion);

    void insertProvided(const Package &parentPkg);

    static bool satisfiesDependency(const std::string &dep, const std::string &packageName, const std::string &packageVersion);

    void markPackageAsBroken(const std::string &packageName);

    std::vector<std::string> getPackageFiles(const std::string &packageName);

    bool removePackage(const std::string &packageName);
};

inline Database AnemoDatabase;

#endif //DATABASE_H
