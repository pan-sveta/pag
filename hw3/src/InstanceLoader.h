#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>

struct Task {
    int n;
    int processTime;
    int releaseTime;
    int deadline;
};

typedef std::vector<Task> TaskList;

TaskList LoadInstance(const std::string& path);
void writeInfeasible(const std::string& path);
void writeFeasible(const std::string& path, std::vector<int> list);