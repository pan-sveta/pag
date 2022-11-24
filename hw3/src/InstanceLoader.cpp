#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>
#include <sstream>
#include <mpi.h>
#include "InstanceLoader.h"

TaskList LoadInstance(const std::string &path) {
    std::vector<Task> tasks(0);

    std::ifstream inputFile;
    inputFile.open(path);

    if (!inputFile) {
        std::cerr << "Input file not found!";
    }

    std::string line;

    std::getline(inputFile, line);
    int taskCount = std::stoi(line);

    for (int i = 0; i < taskCount; ++i) {
        std::getline(inputFile, line);
        std::stringstream ss(line);

        int p, r, d;
        ss >> p >> r >> d;

        Task task{
                .processTime = p,
                .releaseTime= r,
                .deadline = d
        };

        tasks.push_back(task);
    }

    return tasks;
}
