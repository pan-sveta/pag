//
// Created by stepa on 24.11.2022.
//

#include <mpi.h>
#include "Worker.h"
#include "Comm.h"

Worker::Worker(const int &myRankInput, const int &worldSizeInput) {
    this->worldSize = worldSizeInput;
    this->myRank = myRankInput;
}

void Worker::InitialTasksDistribution(const std::string &path) {
    auto MPITaskType = CreateMpiTaskType();

    //Load the instance on root CPU
    if (myRank == 0) {
        tasks = LoadInstance(path);
        task_count = tasks.size();
    }
    //Broadcast the number of tasks
    MPI_Bcast(&task_count, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //If not root -> allocate the task buffer size
    if (myRank != 0)
        tasks = TaskList(task_count);

    //Broadcast all tasks
    MPI_Bcast(&tasks[0], tasks.size(), MPITaskType, 0, MPI_COMM_WORLD);
}

void Worker::InitialJobDistribution() {
    for (int i = 0; i < task_count; ++i) {
        int destination = i % worldSize;

        if (destination != 0) {
            SendInitialTask(destination, i);
        } else {
            std::vector<int> sch(1, i);
            Schedule schedule(tasks, sch);
            backlog.push(schedule);
        }
    }
}


void Worker::WorkingLoop() {
    while (true) {
        if (backlog.empty()) {
            auto receivedSchedule = ReceiveSchedule();
            Schedule schedule(tasks, receivedSchedule);
            schedule.print(myRank);
        }

        auto top = backlog.top();
        backlog.pop();


    }
}





