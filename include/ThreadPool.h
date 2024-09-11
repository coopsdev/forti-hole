//
// Created by Cooper Larson on 5/20/24.
//

#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include "TaskWrapper.h"
#include <condition_variable>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>


class ThreadPool {
    std::atomic<bool> stopFlag;
    unsigned int numThreads;
    std::vector<std::thread> threads;
    std::condition_variable cv;
    std::queue<TaskWrapper> queue;
    std::mutex mutex;

    void worker();

public:
    ThreadPool();
    ~ThreadPool();

    ThreadPool(const ThreadPool& rhs);

    ThreadPool& operator=(const ThreadPool& rhs);

    void stop();
    void submit(TaskWrapper& task);
};

#endif // THREAD_MANAGER_H

