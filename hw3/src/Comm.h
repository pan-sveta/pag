//
// Created by stepa on 24.11.2022.
//

#ifndef HW3_COMM_H
#define HW3_COMM_H

#include <vector>

//Tags
const int MYTAG_SCHEDULE_SEND = 2;
const int MYTAG_TOKEN_PASSING = 4;
const int MYTAG_END = 6;
const int RED_TOKEN = 10;
const int GREEN_TOKEN = 20;
const int NO_TOKEN = -1;

//Types
MPI_Datatype CreateMpiTaskType();

//Communication
void SendInitialTask(const int& destination, const int &taskId);

void SendSchedule(const int& destination, const std::vector<int> &schedule);

void PassToken(const int& destination, int token);

std::vector<int> ReceiveSchedule();
bool IsScheduleAvailable();


#endif //HW3_COMM_H