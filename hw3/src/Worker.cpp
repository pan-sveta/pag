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

    bestSolution = std::vector<int>(taskCount, -1);

    //If not root -> allocate the task buffer size
    if (myRank != 0)
        tasks = TaskList(taskCount);

    //Broadcast all tasks
    MPI_Bcast(&tasks[0], tasks.size(), MPITaskType, 0, MPI_COMM_WORLD);
}

void Worker::InitialJobDistribution() {

    for (int i = 0; i < taskCount; ++i) {
        int destination = i % worldSize;

        /*
        SendInitialTask(destination, i);*/


        Schedule schedule(tasks, i);

        if (destination != 0) {
            SendSchedule(destination, schedule);
        } else {
            backlog.push(schedule);
        }
    }
}


void Worker::WorkingLoop() {
    while (cpuAlive) {
        if (backlog.empty())
            Idling();
        else
            Work();

    }
}

void Worker::Work() {
    MPI_Request request;
    int ubc = UB;
    //TODO: CHECK
    MPI_Iallreduce(&ubc, &UB, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD, &request);

    //Working
    Schedule top = backlog.top();
    backlog.pop();

    ProcessSchedule(top);
}

void Worker::ProcessSchedule(const Schedule &schedule) {
    //std::cout << "Proccessing... ";
    //schedule.print(myRank);

    if (schedule.validate(UB)) {
        if (schedule.isSolution()) {
            //I have a solution
            if (schedule.getLength() < UB) {
                UB = schedule.getLength();
                //std::cout << "New solution discovered with ub: " << UB << std::endl;
                schedule.print(myRank);
                bestSolution = schedule.getScheduledIndex();
                //TODO: Broadcast
            }

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

void Worker::Idling() {
    HandleTokenPassing();

    MPI_Status status;
    MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

    switch (status.MPI_TAG) {
        case MYTAG_SCHEDULE_SEND: {
            HandleScheduleReceive();
            break;
        }
        case MYTAG_TOKEN_PASSING: {
            HandleTokenReceive();
            break;
        }
        case MYTAG_END: {
            HandleEnd();
            break;
        }
    }
}

void Worker::HandleTokenPassing() {
    //I am root and I worked
    if ((myRank == 0) && (worldSize > 1) && !isGreen) {
        isGreen = true;
        std::cout << "TOKEN:: " << "CPU " << myRank << " passing green token " << std::endl;
        PassToken(myRank + 1, GREEN_TOKEN);
    }

    //I have a token
    if (token != NO_TOKEN && myRank != 0) {
        if (isRed) {
            PassToken((myRank + 1) % worldSize, RED_TOKEN);
            std::cout << "TOKEN:: " << "CPU " << myRank << " passing red token to " << (myRank + 1) % worldSize
                      << std::endl;
        }

        PassToken((myRank + 1) % worldSize, token);
        std::cout << "TOKEN:: " << "CPU " << myRank << " passing token as is to " << (myRank + 1) % worldSize
                  << std::endl;

        isGreen = true;
        token = NO_TOKEN;
    }
}

void Worker::HandleTokenReceive() {
    MPI_Status status;
    int recToken;

    MPI_Recv(&recToken, 1, MPI_INT, MPI_ANY_SOURCE, MYTAG_TOKEN_PASSING, MPI_COMM_WORLD, &status);
    if (recToken == GREEN_TOKEN) {
        std::cout << "TOKEN:: " << "CPU " << myRank << " received green token" << std::endl;
        token = GREEN_TOKEN;
    }

    if (recToken == RED_TOKEN) {
        std::cout << "TOKEN:: " << "CPU " << myRank << " received red token" << std::endl;
        token = RED_TOKEN;
    }

    if (myRank == 0) {
        std::cout << "This is the end " << std::endl;
        for (int i = 0; i < worldSize; i++) {
            MPI_Request request;
            MPI_Isend(nullptr, 0, MPI_INT, i, MYTAG_END, MPI_COMM_WORLD, &request);
        }
    }
}

void Worker::HandleScheduleReceive() {
    auto schedule = ReceiveSchedule(tasks);


    std::cout << "RECEIVED:: On CPU " << myRank << " received ";
    schedule.print(myRank);

    backlog.push(schedule);
}

void Worker::HandleEnd() {
    std::cout << "DYING:: On CPU " << myRank << " received" << std::endl;

    if (myRank != 0)
        MPI_Send(&bestSolution[0], taskCount, MPI_INT, 0, 23, MPI_COMM_WORLD);
    else {
        std::vector<std::vector<int>> bufik(worldSize, std::vector<int>(taskCount, -2));
        bufik[0] = bestSolution;
        MPI_Status status;

        for (int i = 1; i < worldSize; ++i) {
            MPI_Recv(&bufik[i][0], taskCount, MPI_INT, i, 23, MPI_COMM_WORLD, &status);
        }

        int ind = 0;
        for (const std::vector<int> &cpu: bufik) {
            std::cout << "CPU " << ind << ": ";
            for (int task: cpu) {
                std::cout << "T" << task + 1 << " ";
            }
            std::cout << std::endl;
            ind++;
        }
    }

    cpuAlive = false;
}




