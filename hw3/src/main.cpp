#include <iostream>
#include "InstanceLoader.h"

int main(int argc, char **argv) {
    std::string inputPath = argv[1];
    std::string outputPath = argv[2];

    auto instance = LoadInstance(inputPath);

    return 0;
}
