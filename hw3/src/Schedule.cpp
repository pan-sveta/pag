//
// Created by stepa on 24.11.2022.
//

#include <vector>
#include <algorithm>
#include <numeric>
#include <iostream>
#include "Schedule.h"
#include <sstream>


void Schedule::append(int el) {
    scheduled.push_back(el);
    pointer++;
    notScheduled.erase(std::remove(notScheduled.begin(), notScheduled.end(), el), notScheduled.end());
}

void Schedule::print(const int &myRank) {
    std::stringstream ss;

    ss << "CPU" << myRank;

    ss << " schedule - Scheduled: ";
    for (int i = 0; i < pointer; ++i)
        ss << "T" << scheduled[i] << " ";
    ss << "Not scheduled: ";
    for (auto i: notScheduled)
        ss << "T" << i << " ";
    ss << std::endl;

    std::cout << ss.str();
}

Schedule::Schedule(const TaskList &_taskList) : taskList(_taskList), pointer(0) {
    scheduled = std::vector<int>(0, -1);
    notScheduled = std::vector<int>(taskList.size(), -1);

    std::iota(notScheduled.begin(), notScheduled.end(), 0);
}

Schedule::Schedule(const TaskList &_taskList, const std::vector<int> &initialState) : taskList(_taskList), pointer(0) {
    scheduled = std::vector<int>(0, -1);
    notScheduled = std::vector<int>(taskList.size(), -1);

    std::iota(notScheduled.begin(), notScheduled.end(), 0);

    for (int el: initialState) {
        append(el);
    }
}
