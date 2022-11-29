//
// Created by stepa on 24.11.2022.
//

#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include "Schedule.h"
#include <sstream>


void Schedule::print(const int &myRank) const {
    std::stringstream ss;

    ss << "CPU" << myRank;

    ss << " schedule - Scheduled: ";
    for (auto i: scheduled)
        ss << "T" << i->n << "(" << i->processTime << "," << i->releaseTime << "," << i->deadline << ") ";
    ss << "Not scheduled: ";
    for (auto i: notScheduled)
        ss << "T" << i->n << "(" << i->processTime << "," << i->releaseTime << "," << i->deadline << ") ";

    ss << "with length " << length;

    ss << std::endl;

    std::cout << ss.str();
}

Schedule::Schedule(const TaskList &_taskList, int initialTask) {
    scheduled = std::vector<const Task *>(0);
    notScheduled = std::vector<const Task *>(0);

    for (int i = 0; i < _taskList.size(); ++i) {
        if (_taskList[i].n == initialTask)
            scheduled.push_back(&_taskList[i]);
        else
            notScheduled.push_back(&_taskList[i]);
    }

    length = calculateLength();
}

Schedule::Schedule(const TaskList &_taskList, const std::vector<int> &_scheduled,
                   const std::vector<int> &_notScheduled) {
    scheduled = std::vector<const Task *>(0);
    notScheduled = std::vector<const Task *>(0);

    for (auto n: _scheduled)
        scheduled.push_back(&_taskList[n]);

    for (auto n: _notScheduled)
        notScheduled.push_back(&_taskList[n]);

    length = calculateLength();
}

Schedule::Schedule(const Schedule &_schedule, const Task *_task) {
    scheduled = _schedule.scheduled;
    notScheduled = _schedule.notScheduled;

    notScheduled.erase(std::remove(notScheduled.begin(), notScheduled.end(), _task), notScheduled.end());
    scheduled.push_back(_task);

    length = std::max(_task->releaseTime, _schedule.length) + _task->processTime;
    //length = calculateLength();
}

std::vector<const Task *> Schedule::getScheduled() const {
    return scheduled;
}

std::vector<const Task *> Schedule::getNotScheduled() const {
    return notScheduled;
}

bool Schedule::isValid(const int &UB) const {

    //Missed deadline
    for (auto task: notScheduled) {
        if (std::max(length, task->releaseTime) + task->processTime > task->deadline)
            return false;
    }

    //Violated upper bound
    if (!notScheduled.empty()) {
        int min = INT32_MAX;
        int sum = 0;
        for (auto task: notScheduled) {
            min = std::min(min, task->releaseTime);
            sum += task->processTime;
        }
        int max = std::max(length, min);
        int LB = max + sum;
        if (LB >= UB)
            return false;
    }

    return true;
}

int Schedule::calculateLength() {
    int len = 0;

    for (auto task: scheduled) {
        len = std::max(task->releaseTime, len) + task->processTime;
    }

    return len;
}

int Schedule::getLength() const {
    return length;
}

bool Schedule::isSolution() const {
    return notScheduled.empty();
}

std::vector<int> Schedule::getScheduledIndex() const {
    std::vector<int> vec;

    for (auto task: scheduled) {
        vec.push_back(task->n);
    }

    return vec;
}

std::vector<int> Schedule::getNotScheduledIndex() const {
    std::vector<int> vec;

    for (auto task: notScheduled)
        vec.push_back(task->n);

    return vec;
}

bool Schedule::isOptimal() const{
    int min = INT32_MAX;
    for(auto task : notScheduled)
        if (min > task->releaseTime)
            min = task->releaseTime;

    return length <= min;
}




