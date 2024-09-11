//
// Created by Cooper Larson on 7/1/24.
//

#include "include/TaskWrapper.h"
#include "include/Task.h"
#include <functional>

TaskWrapper::TaskWrapper(std::unique_ptr<Task> t) : task(std::move(t)) {}

TaskWrapper::TaskWrapper(TaskWrapper &&other) noexcept
    : task(std::move(other.task)), promise(std::move(other.promise)) {}

TaskWrapper &TaskWrapper::operator=(TaskWrapper &&other) noexcept {
    if (this != &other) {
        task = std::move(other.task);
        promise = std::move(other.promise);
    }
    return *this;
}

void TaskWrapper::operator()() {
    try {
        std::function<ExpectedFuture()> func = [this]() mutable -> ExpectedFuture { return task->operator()(); };
        if (func) promise.set_value(func());
    } catch (...) {
        promise.set_exception(std::current_exception());
    }
}

std::future<ExpectedFuture> TaskWrapper::getFuture() { return promise.get_future(); }
