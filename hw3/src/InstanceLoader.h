#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>

struct Task {
    int p;
    int r;
    int d;
};

struct Instance {
    int taskCount;
    std::vector<Task> tasks;

};

Instance LoadInstance(const std::string& path);