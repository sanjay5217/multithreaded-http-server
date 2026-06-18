#include "../includes/thread-pool.hpp"

ThreadPool::ThreadPool(int size, Router& router) {
    this->flag = true;
    for (int i=0; i < size; i++) {
        this->workers.emplace_back(&ThreadPool::conduct_job, this, std::ref(router));
    }
}

void ThreadPool::push(int client_fd) {
    std::unique_lock<std::mutex> locker{this->mutex_lock};
    this->client_queue.push(client_fd);
    this->cond_variable.notify_one();
}

void ThreadPool::conduct_job(Router& router) {
    while(this->flag) {
        std::unique_lock<std::mutex> locker{this->mutex_lock};
        this->cond_variable.wait(locker, 
            [this]() {return !client_queue.empty() || !flag;});
        if (client_queue.empty()) {
            return;
        }
        int client_fd = this->client_queue.front();
        this->client_queue.pop();
        locker.unlock();
        handle_client(client_fd, router);
    }
}

ThreadPool::~ThreadPool() {
    this->flag=false;
    this->cond_variable.notify_all();
    for (auto& t : this->workers) {
        if (t.joinable()) { t.join(); }
    }
}

void handle_client(int client_fd, Router& router) {
        Socket client_socket{client_fd};

        std::string req_msg{};
        req_msg = client_socket.get_message();
        string_dict request_map{extract_string(req_msg)};

        if (request_map.empty()) { 
            return; 
        }

        httpRequest request{request_map["method"], request_map["path"], request_map["version"]};
        request.fill_headers(request_map);

        if (request.get_method() == "POST" && request.get_path() != "/compute") {
            string_dict map{request.get_header()};
            std::cout << map["Content-Length"] << std::endl;
            int content_length{std::stoi(request.get_header()["Content-Length"])};
            request.set_body(client_socket.get_message(content_length));
        }

        std::cout << req_msg << std::endl;
        httpRequest &r_req = request;

        httpResponse res{router.exec_handler(r_req)};
        client_socket.send_message(res.finish_res());
}