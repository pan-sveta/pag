//
// Created by stepa on 24.11.2022.
//

#include <mpi.h>
#include "Worker.h"
#include "Comm.h"

Worker::Worker(const int &myRankInput, const int &worldSizeInput, const std::string &outputPath) {
    this->worldSize = worldSizeInput;
    this->myRank = myRankInput;
    this->outputPath = outputPath;

    gen = std::mt19937_64(rd());
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

    int m = 0;
    for (auto task: tasks)
        m = std::max(m, task.deadline);
    UB = m + 1;
}

void Worker::InitialJobDistribution() {
//    std::cout << "ASSIGNED INITAL TASKS: cpu " << myRank << " got: ";
    for (int i = 0; i < taskCount; ++i) {
        if (i % worldSize == myRank) {
            assignedRootTasks.push(i);
//            std::cout << i << ", ";
        }
    }
//    std::cout << std::endl;
}


void Worker::WorkingLoop() {
    while (cpuAlive) {
        if (backlog.empty() && !assignedRootTasks.empty()) {
            int task = assignedRootTasks.top();
            assignedRootTasks.pop();

            backlog.emplace_back(tasks, task);
        }

        if (backlog.empty())
            Idling();
        else
            Work();

    }
}

void Worker::Work() {
    firstLoop = true;

    MPI_Status status;
    int probeFlag;
    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_SEND_NEW_UB, MPI_COMM_WORLD, &probeFlag, &status);
    if (probeFlag) {
        int newUB;
        MPI_Recv(&newUB,1,MPI_INT,status.MPI_SOURCE,MYTAG_SEND_NEW_UB,MPI_COMM_WORLD,&status);

        if(newUB < UB)
            UB = newUB;
    }

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_NEW_OPTIMAL, MPI_COMM_WORLD, &probeFlag, &status);
    if (probeFlag) {
        backlog.clear();
        assignedRootTasks = std::stack<int>();
        return;
    }


    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &probeFlag, &status);

    //Working
    Schedule top = backlog.front();
    backlog.pop_front();

    ProcessSchedule(top);


    if (probeFlag) {
        MPI_Recv(nullptr, 0, MPI_INT, status.MPI_SOURCE, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &status);

        if (backlog.empty()) {
            bool res = false;
            MPI_Send(&res, 1, MPI_CXX_BOOL, status.MPI_SOURCE, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD);

            return;
        }

        bool res = true;
        MPI_Send(&res, 1, MPI_CXX_BOOL, status.MPI_SOURCE, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD);

        if (myRank > status.MPI_SOURCE)
            isRed = true;

        std::vector<Schedule> toSend(0);

        toSend.push_back(backlog.back());
        backlog.pop_back();

        SendSchedules(status.MPI_SOURCE, toSend);
    }
}

void Worker::ProcessSchedule(const Schedule &schedule) {
/*    std::cout << "Proccessing... ";
    schedule.print(myRank);*/

    if (schedule.isValid(UB)) {
        if (schedule.isSolution()) {
            //I have a solution
            if (schedule.getLength() < UB) {
                UB = schedule.getLength();
                bestSchedule = schedule;

                for (int i = 0; i < worldSize; ++i) {
                    MPI_Request request;
                    if (i != myRank)
                        MPI_Isend(&UB, 1, MPI_INT, i, MYTAG_SEND_NEW_UB, MPI_COMM_WORLD, &request);
                }
            }

        } else {
            if (schedule.isOptimal()) {
                backlog = std::deque<Schedule>();

                for (int i = 0; i < worldSize; ++i) {
                    MPI_Request request;
                    if (i != myRank)
                        MPI_Isend(nullptr, 0, MPI_INT, i, MYTAG_NEW_OPTIMAL, MPI_COMM_WORLD, &request);
                }
            }

            for (auto x : schedule.getNotScheduled()) {
                backlog.push_front(Schedule(schedule, x));
            }
        }
    }
}

void Worker::Idling() {
    if (myRank == 0 && firstLoop)
        PassToken((myRank + 1) % worldSize, GREEN_TOKEN);

    MPI_Status status;
    int flag = 0;

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_END, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        HandleIdleEnd();
    }

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_TOKEN_PASSING, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        HandleIdleTokenReceive();
    }

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_SCHEDULES_SEND, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        HandleIdleScheduleReceive();
    }

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        HandleIdleJobRequest(status.MPI_SOURCE);
    }

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        HandleIdleJobResponse(status.MPI_SOURCE);
        return;
    }

    if (!jobRequestPending)
        AskForJob();

    firstLoop = false;
}

void Worker::HandleIdleEnd() {
    MPI_Request request;

    if (myRank != 0)
        if (bestSchedule)
            SendSchedule(0, bestSchedule.value());
        else
            MPI_Isend(nullptr, 0, MPI_INT, 0, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &request);
    else { // On root
        std::vector<Schedule> results(0);

        //Apend root's solution
        if (bestSchedule)
            results.push_back(bestSchedule.value());

        //Receive solution from everybody
        for (int source = 1; source < worldSize; ++source) {
            MPI_Status status;
            int number_amount;
            MPI_Probe(source, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_INT, &number_amount);

            //If it is empty pass
            if (number_amount < 1)
                continue;

            //If not empty decode
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


        //If results not empty process them to feasible solution
        if (!results.empty()) {
            //Find the one with smallest length
            int smallestIndex = 0;
            int smallestLength = results[0].getLength();
            for (int i = 1; i < results.size(); ++i) {
                if (results[i].getLength() < smallestLength) {
                    smallestIndex = i;
                    smallestLength = results[i].getLength();
                }
            }

            std::vector<int> orda(taskCount, -69);

            int foreo = 0;
            for (auto task: results[smallestIndex].getScheduled()) {
                orda[task->n] = foreo;
                foreo += task->processTime;
            }

            writeFeasible(outputPath, orda);

        } else {
            writeInfeasible(outputPath);
        }


    }

    cpuAlive = false;
}

void Worker::HandleIdleTokenReceive() {
    MPI_Status status;
    int recToken;
    MPI_Recv(&recToken, 1, MPI_INT, MPI_ANY_SOURCE, MYTAG_TOKEN_PASSING, MPI_COMM_WORLD, &status);

    if (myRank != 0) {
        if (isRed)
            PassToken((myRank + 1) % worldSize, RED_TOKEN);
        else
            PassToken((myRank + 1) % worldSize, recToken);
    } else {
        if (recToken == GREEN_TOKEN) {
            for (int i = 0; i < worldSize; i++) {
                //MPI_Request request;
                MPI_Send(nullptr, 0, MPI_INT, i, MYTAG_END, MPI_COMM_WORLD);
            }
        } else {
            PassToken(1, GREEN_TOKEN);
        }
    }

    isRed = false;
}

void Worker::HandleIdleScheduleReceive() {
    auto schedule = ReceiveSchedules(tasks);

    for (const auto &x: schedule)
        backlog.push_back(x);
}

void Worker::HandleIdleJobRequest(int source) {
    MPI_Status status;
    MPI_Recv(nullptr, 0, MPI_INT, source, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &status);

    MPI_Request request;
    bool res = false;
    MPI_Isend(&res, 1, MPI_CXX_BOOL, source, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD, &request);
}

void Worker::HandleIdleJobResponse(int source) {
    MPI_Status status;
    bool res;
    MPI_Recv(&res, 1, MPI_CXX_BOOL, source, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD, &status);

    jobRequestPending = false;

    //TODO: Still don't know why this muset be commented out
    /*if (res) {
        std::cout << myRank << " is sadly waiting dor his promised schedule" << std::endl;
        auto gift = ReceiveSchedule(tasks);
        backlog.push(gift);
        std::cout << myRank << " is happy cause he received his promised schedule" << std::endl;
    }*/
}

void Worker::AskForJob() {
    int destination = myRank;

    while (myRank == destination) {
        destination = dis(gen) % worldSize;
    }

    MPI_Request request;
    MPI_Isend(nullptr, 0, MPI_INT, destination, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &request);
    jobRequestPending = true;
}





