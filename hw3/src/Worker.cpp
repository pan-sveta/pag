//
// Created by stepa on 24.11.2022.
//

#include <mpi.h>
#include "Worker.h"
#include "Comm.h"

Worker::Worker(const int &myRankInput, const int &worldSizeInput, const std::string& outputPath) {
    this->worldSize = worldSizeInput;
    this->myRank = myRankInput;
    this->outputPath = outputPath;
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
                bestSchedule = schedule;

                std::cout << "New solution discovered with ub: " << UB << std::endl;
                schedule.print(myRank);
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
    MPI_Request request;


    if (myRank != 0)
        if (bestSchedule)
            SendSchedule(0, bestSchedule.value());
        else
            MPI_Isend(nullptr, 0, MPI_INT, 0, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &request);
    else {
        std::vector<Schedule> results(0);

        if (bestSchedule)
            results.push_back(bestSchedule.value());

        for (int source = 1; source < worldSize; ++source) {
            MPI_Status status;
            int number_amount;
            MPI_Probe(source, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_INT, &number_amount);

            if (number_amount < 1)
                continue;

            std::vector<int> message(number_amount);
            MPI_Recv(&message[0], number_amount, MPI_INT, status.MPI_SOURCE, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD,
                     &status);

            std::vector<int> S;
            std::vector<int> N;

            bool flag = false;
            for (auto part: message) {
                if (part == -1)
                    flag = true;
                else if (!flag)
                    S.push_back(part);
                else
                    N.push_back(part);
            }

            results.emplace_back(tasks, S, N);
        }


        if(!results.empty()){
            int index = 0;
            int len = results[0].getLength();
            for (int i = 1; i < results.size(); ++i) {
                if (results[i].getLength() < len) {
                    index = i;
                    len = results[i].getLength();
                }
            }

            std::cout << "FINAL RESULTS:" << std::endl;
            results[index].print(myRank);

            std::vector<int> orda(taskCount, -69);

            int foreo = 0;
            for (auto task: results[index].getScheduled()) {
                orda[task->n] = foreo;
                foreo += task->processTime;
            }

            writeFeasible(outputPath, orda);

        } else{
            std::cout << "INFESABLE" << std::endl;
            writeInfeasible(outputPath);
        }


    }

    cpuAlive = false;
}




