//
// Created by stepa on 24.11.2022.
//

#include <mpi.h>
#include "Comm.h"

#include "InstanceLoader.h"

MPI_Datatype CreateMpiTaskType() {
    MPI_Datatype task_type;
    int lengths[3] = {1, 1, 1};

    MPI_Aint displacements[3];
    Task dummy_task = {};
    MPI_Aint base_address;
    MPI_Get_address(&dummy_task, &base_address);
    MPI_Get_address(&dummy_task.processTime, &displacements[0]);
    MPI_Get_address(&dummy_task.releaseTime, &displacements[1]);
    MPI_Get_address(&dummy_task.deadline, &displacements[2]);
    displacements[0] = MPI_Aint_diff(displacements[0], base_address);
    displacements[1] = MPI_Aint_diff(displacements[1], base_address);
    displacements[2] = MPI_Aint_diff(displacements[2], base_address);
    MPI_Datatype types[3] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Type_create_struct(3, lengths, displacements, types, &task_type);
    MPI_Type_commit(&task_type);

    return task_type;
}

void SendInitialTask(const int& destination, const int &taskId) {
    MPI_Send(&taskId, 1, MPI_INT, destination, COM_SEND_SCHEDULE, MPI_COMM_WORLD);
}

void SendSchedule(const int& destination, const std::vector<int> &schedule) {
    MPI_Send(&schedule, schedule.size(), MPI_INT, destination, COM_SEND_SCHEDULE, MPI_COMM_WORLD);
}

std::vector<int> ReceiveSchedule() {
    MPI_Status status;
    int number_amount;
    MPI_Probe(MPI_ANY_SOURCE, COM_SEND_SCHEDULE, MPI_COMM_WORLD, &status);
    MPI_Get_count(&status, MPI_INT, &number_amount);
    std::vector<int> buffer(number_amount, -1);

    MPI_Recv(&buffer[0], number_amount, MPI_INT, MPI_ANY_SOURCE, COM_SEND_SCHEDULE,
             MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    return buffer;
}


