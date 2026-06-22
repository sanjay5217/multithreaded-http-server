#include "../include/thread-pool.hpp"

ThreadPool::ThreadPool(int size, Router& router) {
    this->flag = true;
    this->workers.reserve(size);

    for (int i = 0; i < size; i++) {
        this->workers.emplace_back(router, [this](){ this->cv.notify_one(); });
    }

    for (auto& t : this->workers) {
        t.assign_work();
    }
}

void ThreadPool::push(int client_fd) {
    int total_capacity = this->workers[0].get_capacity();

    while (this->flag) {
        std::unique_lock<std::mutex> locker{this->mutex_lock};

        int least_occupied = 0;
        for (size_t i = 1; i < this->workers.size(); i++) {
            if (this->workers[i].get_occupancy() < this->workers[least_occupied].get_occupancy()) {
                least_occupied = i;
            }
        }

        if (this->workers[least_occupied].reserve_slot()) {
            this->workers[least_occupied].enqueue_client(client_fd);
            char byte = 'X';
            write(this->workers[least_occupied].get_write_fd(), &byte, 1);
            locker.unlock();
            return;
        } else {
            this->cv.wait(locker, [this, total_capacity]() {
                if (!this->flag) { return true; }
                for (auto& w : this->workers) {
                    if (w.get_occupancy() < total_capacity) { return true; }
                }
                return false;
            });
        }
    }
}


ThreadPool::~ThreadPool() {
    this->flag=false;
    this->cv.notify_all();
}