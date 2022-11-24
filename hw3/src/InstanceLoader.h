#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>

struct Task {
    int processTime;
    int releaseTime;
    int deadline;
};

typedef std::vector<Task> TaskList;

TaskList LoadInstance(const std::string& path);