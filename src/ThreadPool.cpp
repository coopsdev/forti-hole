//
// Created by Cooper Larson on 5/20/24.
//

#include "include/ThreadPool.h"
#include <algorithm>
#include <utility>

ThreadPool::ThreadPool()
        : stopFlag(std::atomic<bool>(false)),
          numThreads(std::thread::hardware_concurrency()),
          threads(numThreads) {
    for (auto& thread : threads) thread = std::thread(&ThreadPool::worker, this);
}

ThreadPool::~ThreadPool() {
    stop();
    for (auto& thread : threads) if (thread.joinable()) thread.join();
}

ThreadPool::ThreadPool(const ThreadPool &rhs)
    : stopFlag(rhs.stopFlag.load()),
    numThreads(rhs.numThreads),
    threads(numThreads),
    cv() {}

ThreadPool& ThreadPool::operator=(const ThreadPool &rhs) {
    if (this != &rhs) {
        stopFlag = rhs.stopFlag.load();
        numThreads = rhs.numThreads;
    }
    return *this;
}

void ThreadPool::stop() {
    stopFlag.store(true);
    cv.notify_all();
}

void ThreadPool::submit(TaskWrapper &task) {
    std::scoped_lock lock(mutex);
    queue.push(std::move(task));
    cv.notify_one();
}

void ThreadPool::worker() {
    while (!stopFlag.load()) {
        std::unique_ptr<TaskWrapper> task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this]() -> bool {
                return !queue.empty() || stopFlag.load();
            });

            if (stopFlag.load() && queue.empty()) {
                return;
            }

            if (!queue.empty()) {
                task = std::make_unique<TaskWrapper>(std::move(queue.front()));
                queue.pop();
            }
        }

        if (task) {
            (*task)();
        }
    }
}
