//
// Created by cv2 on 03.02.2025.
//

#ifndef PACKAGEBUILDER_H
#define PACKAGEBUILDER_H

#include <filesystem>
#include <string>
#include <unordered_map>

class PackageBuilder {
public:
    explicit PackageBuilder(const std::filesystem::path& package_dir);

    bool build(bool use_binary = false);
    bool install();

    void parse_metadata();

    bool check_dependency(const std::string &dep);

private:
    struct PackageMetadata {
        std::string name;
        std::string version;
        std::vector<std::string> dependencies;
        std::string source_url;
        std::string source_sha256;
    };

    PackageMetadata metadata_;
    std::filesystem::path build_dir_;

    void fetch_source();
    bool verify_checksum();
    void setup_build_environment();

    void create_apkg();
};

#endif //PACKAGEBUILDER_H
