#include "cli/cli.hpp"

int main(int argc, char** argv) {
    anemonix::Cli cli;
    return cli.execute(argc, argv);
}