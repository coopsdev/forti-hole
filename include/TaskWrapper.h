//
// Created by Cooper Larson on 5/28/24.
//

#ifndef TSP_TASKWRAPPER_H
#define TSP_TASKWRAPPER_H

#include <memory>
#include <future>
#include "Task.h"

struct Task;

class TaskWrapper {
    std::unique_ptr<Task> task;
    std::promise<ExpectedFuture> promise{};

public:
    TaskWrapper() = default;
    explicit TaskWrapper(std::unique_ptr<Task> t);
    TaskWrapper(const TaskWrapper& rhs) = delete;
    TaskWrapper(TaskWrapper&& other) noexcept;

    TaskWrapper& operator=(const TaskWrapper& rhs) = delete;
    TaskWrapper& operator=(TaskWrapper&& other) noexcept;
    void operator()();

    std::future<ExpectedFuture> getFuture();
};

#endif // TSP_TASKWRAPPER_H