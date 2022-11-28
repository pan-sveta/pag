//
// Created by stepa on 24.11.2022.
//

#ifndef HW3_WORKER_H
#define HW3_WORKER_H


#include <stack>
#include "Schedule.h"
#include "Comm.h"
#include <optional>
#include <random>


class Worker {
private:
    std::stack<Schedule> backlog;
    std::stack<int> assignedRootTasks;
    int myRank;
    int worldSize;
    int taskCount;
    TaskList tasks;
    bool cpuAlive = true;

    std::string outputPath;

    int UB = INT32_MAX;
    std::optional<Schedule> bestSchedule;

    bool firstLoop = true;
    bool jobRequestPending = false;

    std::random_device rd;
    std::mt19937_64 gen;
    std::uniform_int_distribution<int> dis;

public:
    int token = NO_TOKEN;
    bool isRed = false;

    Worker(const int &myRankInput, const int &worldSizeInput, const std::string& outputPath);

    void WorkingLoop();

    void InitialJobDistribution();

    void InitialTasksDistribution(const std::string& path);

    void ProcessSchedule(const Schedule &schedule);

    void Idling();

    void HandleIdleTokenReceive();

    void HandleIdleScheduleReceive();

    void HandleIdleJobRequest(int source);

    void HandleIdleEnd();

    void Work();

    void AskForJob();

    void HandleIdleJobResponse(int i);
};


#endif //HW3_WORKER_H
