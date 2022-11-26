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
}

void Worker::InitialJobDistribution() {

    for (int i = 0; i < taskCount; ++i) {
        int destination = i % worldSize;

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
    firstLoop = true;

    MPI_Status status;
    int probeFlag;
    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &probeFlag, &status);


    //TODO: CHECK
    int ubc = UB;
    //MPI_Allreduce(&ubc, &UB, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    //std::cout << "WORKING" << std::endl;

    //Working
    Schedule top = backlog.top();
    backlog.pop();

    ProcessSchedule(top);


    if (probeFlag) {
        MPI_Recv(nullptr, 0, MPI_INT, status.MPI_SOURCE, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &status);

        if (backlog.empty()) {
            /*std::cout << "JOB REQUEST DENIED (stack empty): CPU " << myRank << " does not have job for CPU "
                      << status.MPI_SOURCE << std::endl;*/

            bool res = false;
            MPI_Send(&res, 1, MPI_CXX_BOOL, status.MPI_SOURCE, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD);

            return;
        }

        bool res = true;
        MPI_Send(&res, 1, MPI_CXX_BOOL, status.MPI_SOURCE, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD);

        if (myRank > status.MPI_SOURCE)
            isRed = true;

        Schedule toSend = backlog.top();
        backlog.pop();
        std::cout << "JOB REQUEST ACCEPTED: from " << myRank << " sending to " << status.MPI_SOURCE << " = ";
        toSend.print(myRank);

        SendSchedule(status.MPI_SOURCE, toSend);
    }
}

void Worker::ProcessSchedule(const Schedule &schedule) {
    //std::cout << "Proccessing... ";
    //schedule.print(myRank);

    if (schedule.isValid(UB)) {
        if (schedule.isSolution()) {
            //I have a solution
            if (schedule.getLength() < UB) {
                UB = schedule.getLength();
                bestSchedule = schedule;

                //std::cout << "New solution discovered with ub: " << UB << std::endl;
                //schedule.print(myRank);
            }

        } else {
            //I have to reverse iterate it because of the stack
            if(schedule.isOptimal()) {
                backlog = std::stack<Schedule>();
            }

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
    //std::cout << "IDLE: cpu " << myRank << " is idling" << std::endl;
    if (myRank == 0 && firstLoop)
        PassToken((myRank + 1) % worldSize, GREEN_TOKEN);

    MPI_Status status;
    int flag = 0;

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_END, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        HandleEnd();
    }

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_TOKEN_PASSING, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        HandleTokenReceive();
    }

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        //jobRequestPending = false;
        HandleScheduleReceive();
    }

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        MPI_Recv(nullptr, 0, MPI_INT, status.MPI_SOURCE, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &status);

        MPI_Request request;
        bool res = false;
        MPI_Isend(&res, 1, MPI_CXX_BOOL, status.MPI_SOURCE, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD, &request);
    }

    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD, &flag, &status);
    if (flag) {
        bool res;
        MPI_Recv(&res, 1, MPI_CXX_BOOL, status.MPI_SOURCE, MYTAG_JOB_REQUEST_RESPONSE, MPI_COMM_WORLD, &status);

        jobRequestPending = false;
        if(res){
            std::cout << myRank << " is sadly waiting dor his promised schedule" << std::endl;
            auto gift = ReceiveSchedule(tasks);
            backlog.push(gift);
            std::cout << myRank << " is happy cause he received his promised schedule" << std::endl;
            return;
        }
    }

    if (!jobRequestPending)
        AskForJob();

    firstLoop = false;
}

void Worker::HandleTokenReceive() {
    MPI_Status status;
    int recToken;
    MPI_Recv(&recToken, 1, MPI_INT, MPI_ANY_SOURCE, MYTAG_TOKEN_PASSING, MPI_COMM_WORLD, &status);

    if (recToken == GREEN_TOKEN)
        std::cout << "TOKEN:: " << "CPU " << myRank << " received green token" << std::endl;

    if (recToken == RED_TOKEN)
        std::cout << "TOKEN:: " << "CPU " << myRank << " received red token" << std::endl;

    if (myRank != 0) {
        if (isRed)
            PassToken((myRank + 1) % worldSize, RED_TOKEN);
        else
            PassToken((myRank + 1) % worldSize, recToken);
    } else if (myRank == 0) {
        if (recToken == GREEN_TOKEN) {
            std::cout << "This is the end " << std::endl;
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

void Worker::HandleScheduleReceive() {
    auto schedule = ReceiveSchedule(tasks);

    //std::cout << "RECEIVED:: On CPU " << myRank << " received ";
    //schedule.print(myRank);

    backlog.push(schedule);
}

void Worker::HandleEnd() {
    //std::cout << "DYING:: On CPU " << myRank << " received" << std::endl;
    MPI_Request request;

    //If not root and has some solution -> send it to root
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

            std::cout << "Final result" << std::endl;
            results[smallestIndex].print(myRank);

            std::vector<int> orda(taskCount, -69);

            int foreo = 0;
            for (auto task: results[smallestIndex].getScheduled()) {
                orda[task->n] = foreo;
                foreo += task->processTime;
            }

            writeFeasible(outputPath, orda);

        } else {
            std::cout << "Infeasible" << std::endl;
            writeInfeasible(outputPath);
        }


    }

    cpuAlive = false;
}

void Worker::AskForJob() {
    int destination = myRank;

    while (myRank == destination) {
        destination = dis(gen) % worldSize;
    }

    MPI_Request request;
//    std::cout << "JOB REQUEST: " << myRank << " is asking " << destination << " for a job " << std::endl;
    MPI_Isend(nullptr, 0, MPI_INT, destination, MYTAG_JOB_REQUEST, MPI_COMM_WORLD, &request);
    jobRequestPending = true;
}




