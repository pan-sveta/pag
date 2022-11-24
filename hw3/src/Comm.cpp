//
// Created by stepa on 24.11.2022.
//

#include <mpi.h>
#include "Comm.h"

#include "Schedule.h"


MPI_Datatype CreateMpiTaskType() {
    MPI_Datatype task_type;
    int lengths[4] = {1, 1, 1, 1};

    MPI_Aint displacements[4];
    Task dummy_task = {};
    MPI_Aint base_address;
    MPI_Get_address(&dummy_task, &base_address);
    MPI_Get_address(&dummy_task.n, &displacements[0]);
    MPI_Get_address(&dummy_task.processTime, &displacements[1]);
    MPI_Get_address(&dummy_task.releaseTime, &displacements[2]);
    MPI_Get_address(&dummy_task.deadline, &displacements[3]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);
    displacements[2] = MPI_Aint_diff(displacements[2], base_address);
    displacements[3] = MPI_Aint_diff(displacements[3], base_address);
    MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Type_create_struct(4, lengths, displacements, types, &task_type);
    MPI_Type_commit(&task_type);

    return task_type;
}

void SendSchedule(const int &destination, const Schedule& schedule) {
    MPI_Request request;

    std::vector<int> message;

    for (auto task : schedule.getScheduledIndex())
        message.push_back(task);

    message.push_back(-1);

    for (auto task : schedule.getNotScheduledIndex())
        message.push_back(task);

    MPI_Isend(&message[0], message.size() , MPI_INT, destination, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &request);
}


Schedule ReceiveSchedule(const TaskList &taskList) {
    MPI_Status status;
    int number_amount;


    MPI_Probe(MPI_ANY_SOURCE, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &number_amount);
    std::vector<int> message(number_amount);
    MPI_Recv(&message[0], number_amount , MPI_INT, status.MPI_SOURCE, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &status);

    std::vector<int> S;
    std::vector<int> N;

    bool flag = false;
    for (auto part : message) {
        if (part == -1)
            flag = true;
        else if(!flag)
            S.push_back(part);
        else
            N.push_back(part);
    }

    return Schedule(taskList,S,N);
}

void PassToken(const int &destination, int token) {
    MPI_Send(&token, 1, MPI_INT, destination, MYTAG_TOKEN_PASSING, MPI_COMM_WORLD);
}



