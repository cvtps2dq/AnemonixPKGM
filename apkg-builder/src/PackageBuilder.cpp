//
// Created by cv2 on 03.02.2025.
//

#include <yaml-cpp/yaml.h>
#include "../include /builder/PackageBuilder.hpp"
#include <sys/wait.h>
#include <openssl/sha.h>
#include <sodium.h>

void PackageBuilder::parse_metadata() {
    YAML::Node config = YAML::LoadFile("anemonix.yaml");

    metadata_.name = config["name"].as<std::string>();
    metadata_.version = config["version"].as<std::string>();

    for (const auto& dep : config["deps"])
        metadata_.dependencies.push_back(dep.as<std::string>());
}

bool PackageBuilder::check_dependency(const std::string& dep) {
    // Query /var/lib/anemonix/installed.db
    // Return true if compatible version exists
    return false;
}

void PackageBuilder::setup_build_environment() {
    pid_t pid = fork();
    if (pid == 0) {
        execlp("bwrap", "bwrap",
            "--ro-bind /usr /usr",
            "--dir /build",
            "--chdir /build",
            // ... other namespaces
            "bash", "build.anemonix", nullptr);
        exit(EXIT_FAILURE);
    }
    waitpid(pid, nullptr, 0);
}

void PackageBuilder::create_apkg() {
    std::filesystem::create_directory("binaries");

    // Create tarball with metadata + installed files
    system(fmt::format("tar -czvf {0}-{1}.apkg anemonix.yaml binaries/",
        metadata_.name, metadata_.version).c_str());
}

bool PackageBuilder::verify_checksum() {
    std::vector<uint8_t> file_data = read_file("source.tar.gz");
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(file_data.data(), file_data.size(), hash);

    return hex_encode(hash) == metadata_.source_sha256;
}

bool verify_binary(const std::string& pkg_path, const std::string& pubkey) {
    std::vector<uint8_t> sig = read_file(pkg_path + ".sig");
    std::vector<uint8_t> data = read_file(pkg_path);

    return crypto_sign_verify_detached(
        sig.data(), data.data(), data.size(), pubkey.data()
    ) == 0;
}