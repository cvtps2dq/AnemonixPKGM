//
// Created by cv2 on 03.02.2025.
//

// commands/build.cpp
#include <iostream>
#include <stdexcept>

#include "../../include/cli/cli.h"
#include <sys/wait.h>
#include <unistd.h>
#include <__filesystem/path.h>

void anemonix::Cli::handle_build(int argc, char** argv) {
    if (argc < 3) {
        throw std::runtime_error("Missing package path");
    }

    const std::filesystem::path pkg_path(argv[2]);

    std::cout << "Building package in sandbox...\n";

    pid_t pid = fork();
    if (pid == 0) {
        // Child process: sandboxed build
        execlp("bwrap", "bwrap",
            "--ro-bind /usr /usr",
            "--dev /dev",
            "--proc /proc",
            "--tmpfs /build",
            "--chdir /build",
            "bash", "-c", "echo 'Simulated build step' && sleep 1",
            nullptr);

        exit(EXIT_FAILURE);
    }

    // Wait for build completion
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        std::cout << "Build successful!\n";
    } else {
        throw std::runtime_error("Build failed");
    }
}
