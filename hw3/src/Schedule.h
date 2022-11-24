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

    int calculateLength();


public:
    Schedule(): length(0){};
    Schedule(const TaskList &_taskList, int initialTask);

    Schedule(const TaskList &_taskList, const std::vector<int> &_scheduled, const std::vector<int> &_notScheduled);

    Schedule(const Schedule &_schedule, const Task *_task);

    bool isSolution() const;

    bool validate(const int &UB) const;

    void print(const int &myRank) const;

    std::vector<const Task *> getScheduled() const;
    std::vector<const Task *> getNotScheduled() const;

    std::vector<int> getScheduledIndex() const;

    std::vector<int> getNotScheduledIndex() const;

    int getLength() const;


};


#endif //HW3_SCHEDULE_H
