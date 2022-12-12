//
// Created by stepa on 24.11.2022.
//

#ifndef HW3_COMM_H
#define HW3_COMM_H

#include <vector>
#include "Schedule.h"

//Tags
const int MYTAG_SEND_NEW_UB = 128;
const int MYTAG_SCHEDULE_SEND = 2;
const int MYTAG_SCHEDULES_SEND = 256;
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

void SendSchedules(int destination, std::vector<Schedule> schedules);
std::vector<Schedule> ReceiveSchedules(const TaskList &taskList);

void PassToken(int destination, int token);

void SendNewUB(int source, int UB);

#endif //HW3_COMM_H
