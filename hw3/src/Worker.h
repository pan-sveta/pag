//
// Created by stepa on 24.11.2022.
//

#ifndef HW3_WORKER_H
#define HW3_WORKER_H


#include <stack>
#include "Schedule.h"
#include "Comm.h"

class Worker {
private:
    std::stack<Schedule> backlog;
    int myRank;
    int worldSize;
    int taskCount;
    int UB = INT32_MAX;
    TaskList tasks;
    bool cpuAlive = true;

public:
    int token = NO_TOKEN;
    bool isRed = false;
    bool isGreen = false;

    Worker(const int &myRankInput, const int &worldSizeInput);

    void WorkingLoop();

    void InitialJobDistribution();

    void InitialTasksDistribution(const std::string& path);

    void ProcessSchedule(const Schedule &schedule);

    void Idling();

    void HandleTokenPassing();

    void HandleTokenReceive();

    void HandleScheduleReceive();

    void HandleEnd();

    void Work();
};


#endif //HW3_WORKER_H
