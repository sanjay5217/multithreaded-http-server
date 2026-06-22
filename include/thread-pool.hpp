#pragma once

#include <queue>
#include <vector>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "router.hpp"
#include "utils.hpp"
#include "worker.hpp"

class ThreadPool {
    public:
        ThreadPool(int size, Router& router);
        ~ThreadPool();

        /**
        * @brief Pushes the client into the thread pool. The thread pool
        *        then sends the job to the worker with the last clients.
        *        If all workers are full, this will wait until one worker 
        *        has space in its job queue. 
        *
        * @param client_fd The client file descriptor
        */
        void push(int client_fd);

    private:
        std::atomic<bool> flag;
        std::vector<Worker> workers;
        std::mutex mutex_lock;
        std::condition_variable cv;
};