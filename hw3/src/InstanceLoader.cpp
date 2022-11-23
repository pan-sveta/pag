#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include "InstanceLoader.h"

Instance LoadInstance(const std::string &path) {
    Instance instance;

    std::ifstream inputFile;
    inputFile.open(path);

    if (!inputFile) {
        std::cerr << "Input file not found!";
    }

    std::string line;

    std::getline(inputFile, line);
    instance.taskCount = std::stoi(line);
    instance.tasks = std::vector<Task>(0);

    for (int i = 0; i < instance.taskCount; ++i) {
        std::getline(inputFile, line);
        std::stringstream ss(line);

        int p, r, d;
        ss >> p >> r >> d;

        Task task{
                p = p,
                r = r,
                d = d
        };

        instance.tasks.push_back(task);
    }

    return instance;
}

