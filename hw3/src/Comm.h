//
// Created by stepa on 24.11.2022.
//

#ifndef HW3_COMM_H
#define HW3_COMM_H

#include <vector>

//Tags
const int COM_SEND_SCHEDULE = 2;

//Types
MPI_Datatype CreateMpiTaskType();

//Communication
void SendInitialTask(const int& destination, const int &taskId);

void SendSchedule(const int& destination, const std::vector<int> &schedule);

void PassToken(const int& destination, int token);

std::vector<int> ReceiveSchedule();


#endif //HW3_COMM_H
