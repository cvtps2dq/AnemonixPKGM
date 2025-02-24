//
// Created by cv2 on 2/24/25.
//

#include "Filesystem.h"
#include <archive.h>
#include <archive_entry.h>
#include <cerrno>
#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <colors.h>
#include <config.h>

bool Filesystem::secondPass(const std::string& package_path, std::vector<std::string>& installed_files) {return true;}
