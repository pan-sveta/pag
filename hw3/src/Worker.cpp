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
        taskCount = tasks.size();
    }

    //Broadcast the number of tasks
    MPI_Bcast(&taskCount, 1, MPI_INT, 0, MPI_COMM_WORLD);

    //If not root -> allocate the task buffer size
    if (myRank != 0)
        tasks = TaskList(taskCount);

    //Broadcast all tasks
    MPI_Bcast(&tasks[0], tasks.size(), MPITaskType, 0, MPI_COMM_WORLD);
}

void Worker::InitialJobDistribution() {
    for (int i = 0; i < taskCount; ++i) {
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
    bool hasWorked = false;

    while (true) {
        if (backlog.empty()) {
            //Idling
            auto receivedSchedule = ReceiveSchedule();
            Schedule schedule(tasks, receivedSchedule);
            backlog.push(schedule);
        } else {
            //Working
            hasWorked = true;
            Schedule top = backlog.top();
            backlog.pop();

            ProcessSchedule(top);
        }

    }
}

void Worker::ProcessSchedule(const Schedule &schedule) {
    std::cout << "Proccessing... ";
    schedule.print(myRank);

    if (schedule.validate(UB)) {
        if (schedule.isSolution()) {
            //I have a solution
            if (schedule.getLength() < UB){
                UB = schedule.getLength();
                std::cout << "New UB discovered: " << UB << std::endl;
                //TODO: Broadcast
            }

            std::cout << "Solution: ";
            schedule.print(myRank);
            return;
        } else {
            //I have to reverse iterate it because of the stack
            auto notScheduled = schedule.getNotScheduled();
            for (int i = notScheduled.size() - 1; i > -1; --i) {
                backlog.push(Schedule(schedule, notScheduled[i]));
            }
        }
    } else {
        //std::cout << "INVALID!" << std::endl;
    }
}





