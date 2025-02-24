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
    bool insertPackage(const Package &pkg);
    std::optional<Package> getPackage(const std::string &packageName);
    bool init();
};

inline Database AnemoDatabase;

#endif //DATABASE_H
