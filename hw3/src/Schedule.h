//
// Created by stepa on 24.11.2022.
//

#ifndef HW3_SCHEDULE_H
#define HW3_SCHEDULE_H


#include <vector>
#include "InstanceLoader.h"

class Schedule {
private:
    std::vector<int> scheduled;
    std::vector<int> notScheduled;
    const TaskList& taskList;
    int pointer;


public:
    explicit Schedule(const TaskList& _taskList);
    Schedule(const TaskList& _taskList, const std::vector<int>& initialState);;

    void append(int el);

    void print(const int& myRank);
};


#endif //HW3_SCHEDULE_H
