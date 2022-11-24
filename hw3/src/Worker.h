//
// Created by stepa on 24.11.2022.
//

#ifndef HW3_WORKER_H
#define HW3_WORKER_H


#include <stack>
#include "Schedule.h"

class Worker {
private:
    std::stack<Schedule> backlog;
    int myRank;
    int worldSize;
    int task_count;
    TaskList tasks;
public:
    Worker(const int &myRankInput, const int &worldSizeInput);

    void WorkingLoop();

    void InitialJobDistribution();

    void InitialTasksDistribution(const std::string& path);
};


#endif //HW3_WORKER_H
