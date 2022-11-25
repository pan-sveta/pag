//
// Created by stepa on 24.11.2022.
//

#ifndef HW3_COMM_H
#define HW3_COMM_H

#include <vector>
#include "Schedule.h"

//Tags
const int MYTAG_SCHEDULE_SEND = 2;
const int MYTAG_TOKEN_PASSING = 8;
const int MYTAG_END = 16;
const int MYTAG_JOB_REQUEST = 32;
const int MYTAG_JOB_REQUEST_RESPONSE = 64;
const int RED_TOKEN = 10;
const int GREEN_TOKEN = 20;
const int NO_TOKEN = -1;

//Types
MPI_Datatype CreateMpiScheduleType(int taskCount);
MPI_Datatype CreateMpiTaskType();

//Communication
void SendSchedule(int destination, Schedule schedule);

Schedule ReceiveSchedule(const TaskList& taskList);

void PassToken(int destination, int token);

#endif //HW3_COMM_H
