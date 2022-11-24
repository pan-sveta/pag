//
// Created by stepa on 24.11.2022.
//

#ifndef HW3_SCHEDULE_H
#define HW3_SCHEDULE_H


#include <vector>
#include "InstanceLoader.h"

class Schedule {
private:
    std::vector<const Task *> scheduled;
    std::vector<const Task *> notScheduled;
    int length;
    int UB = -1;

    int calculateLength();


public:
    explicit Schedule(const TaskList &_taskList);

    Schedule(const TaskList &_taskList, const std::vector<int> &initialState);;

    Schedule(const Schedule &_schedule, const Task *_task);

    int getLength() const;
    std::vector<const Task *> getNotScheduled() const;
    bool isSolution() const;

    bool validate(const int& UB) const;

    void print(const int &myRank) const;


    std::vector<int> getVector() const;
};


#endif //HW3_SCHEDULE_H
