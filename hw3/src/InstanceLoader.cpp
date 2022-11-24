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

    FILE *finput;
    finput = fopen(path.c_str(), "r");

    if (!finput) {
        std::cerr << "Input file not found!";
    }

    int taskCount;
    fscanf(finput, "%d", &taskCount);

    for (int i = 0; i < taskCount; ++i) {

        int p, r, d;
        fscanf(finput, "%d %d %d", &p, &r, &d);

        Task task{
                .n = i,
                .processTime = p,
                .releaseTime= r,
                .deadline = d
        };

        tasks.push_back(task);
    }

    return tasks;
}

void writeInfeasible(const std::string &path) {
    FILE *finput;
    finput = fopen(path.c_str(), "w");

    fprintf(finput, "%d", -1);

}

void writeFeasible(const std::string &path, std::vector<int> list) {
    FILE *finput;
    finput = fopen(path.c_str(), "w");

    for (auto el: list) {
        fprintf(finput, "%d\n", el);
    }
}
