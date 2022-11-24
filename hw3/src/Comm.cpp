//
// Created by stepa on 24.11.2022.
//

#include <mpi.h>
#include "Comm.h"

#include "InstanceLoader.h"

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

void SendInitialTask(const int &destination, const int &taskId) {
    MPI_Request request;
    MPI_Isend(&taskId, 1, MPI_INT, destination, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &request);
}

void SendSchedule(const int &destination, const std::vector<int> &schedule) {
    MPI_Request request;
    MPI_Isend(&schedule, schedule.size(), MPI_INT, destination, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &request);
}

bool IsScheduleAvailable() {
    MPI_Status status;
    int flag;
    MPI_Iprobe(MPI_ANY_SOURCE, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &flag, &status);


    if (flag)
        return true;
    else
        return false;
}

std::vector<int> ReceiveSchedule() {

    //TODO Make non-blocking
    MPI_Status status;
    int number_amount;
    MPI_Probe(MPI_ANY_SOURCE, MYTAG_SCHEDULE_SEND, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &number_amount);
    std::vector<int> buffer(number_amount, -1);

    MPI_Recv(&buffer[0], number_amount, MPI_INT, MPI_ANY_SOURCE, MYTAG_SCHEDULE_SEND,
             MPI_COMM_WORLD, &status);

    return buffer;
}

void PassToken(const int &destination, int token) {
    MPI_Send(&token, 1, MPI_INT, destination, MYTAG_TOKEN_PASSING, MPI_COMM_WORLD);
}


