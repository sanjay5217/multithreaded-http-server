#pragma once

#include <queue>
#include <vector>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "router.hpp"
#include "utils.hpp"

void handle_client(int client_fd, Router& router);

class ThreadPool {
    public:
    ThreadPool(int size, Router& router);
    void push(int client_fd);
    ~ThreadPool();

    private:
    std::atomic<bool> flag;
    std::vector<std::thread> workers;
    std::mutex mutex_lock;
    std::condition_variable cond_variable;
    std::queue<int> client_queue;
    void conduct_job(Router& router);
};