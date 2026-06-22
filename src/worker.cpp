#include "../include/worker.hpp"

Worker::Worker(Router& router, std::function<void()> slot_freed) :
    router{router}, capacity{CLIENT_NUMBER}, current{0}, on_slot_freed{slot_freed} {
    int pipe_fd[2];
    pipe(pipe_fd);
    this->alert.read = pipe_fd[0];
    this->alert.write = pipe_fd[1];
    this->kq_fd = kqueue();

    struct kevent change;
    EV_SET(&change, this->alert.read, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    kevent(this->kq_fd, &change, 1, nullptr, 0, nullptr);
    if (this->kq_fd < 0) {
        return;
    }
}

Worker::Worker(Worker&& other) :
    t_worker{std::move(other.t_worker)},
    router{other.router},
    capacity{other.capacity},
    current{other.current.load()},
    kq_fd{other.kq_fd},
    alert{other.alert},
    clients{std::move(other.clients)},
    on_slot_freed{std::move(other.on_slot_freed)},
    client_queue{std::move(other.client_queue)} {
    other.kq_fd = -1;
}

void Worker::enqueue_client(int client_fd) {
    std::lock_guard<std::mutex> lock(this->queue_mutex);
    this->client_queue.push(client_fd);
}

bool Worker::reserve_slot() {
    int expected = this->current.load();
    while (expected < this->capacity) {
        if (this->current.compare_exchange_weak(expected, expected + 1)) {
            return true;
        }
    }
    return false;
}

void Worker::accept_client(int client_fd) {
    struct kevent change;
    EV_SET(&change, client_fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
    kevent(this->kq_fd, &change, 1, nullptr, 0, nullptr);
}

void Worker::remove_client(int client_fd) {
    struct kevent change;
    EV_SET(&change, client_fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
    kevent(this->kq_fd, &change, 1, nullptr, 0, nullptr);
    this->current-=1;
    this->clients.erase(client_fd);
    this->on_slot_freed();
}

int Worker::get_capacity() { return this->capacity; }

int Worker::get_occupancy() { return this->current; }

void Worker::assign_work() { this->t_worker = std::thread(&Worker::handle_client, this); }

int Worker::get_write_fd() { return this->alert.write; }

void Worker::handle_client() {
    struct kevent client_list[CLIENT_NUMBER + 1];
    int num_events;
    while ((num_events = kevent(this->kq_fd, nullptr, 0, client_list, CLIENT_NUMBER + 1, nullptr)) > 0) {
        for (int i = 0; i < num_events; i++) {
            int client_fd = (int)client_list[i].ident;

            // Stage 1: register new clients, or stop if asked to shut down
            if (this->alert.read == client_fd) {
                char indicator;
                read(client_fd, &indicator, 1);
                if (this->should_stop) {
                    return;
                }
                int new_client;
                {
                    std::lock_guard<std::mutex> lock(this->queue_mutex);
                    new_client = this->client_queue.front();
                    this->client_queue.pop();
                }
                this->accept_client(new_client);
                this->clients.emplace(new_client,
                    client_state{Socket{new_client}, request_phase::READHEADER, 0, std::nullopt});
                continue;
            }

            auto it = this->clients.find(client_fd);
            if (it == this->clients.end()) { continue; }
            client_state& cs = it->second;

            // A previously-blocked response just became writable again -- resume it.
            if (client_list[i].filter == EVFILT_WRITE) {
                message_status send_status = cs.socket.send_message("");
                if (send_status != message_status::BLOCKED) {
                    this->remove_client(client_fd);
                }
                continue;
            }

            // Stage 2: deal with existing clients
            message_status status = (cs.phase == request_phase::READBODY)
                ? cs.socket.get_message(cs.content_length)
                : cs.socket.get_message();

            this->handle_status(cs, status, client_fd);
        }
    }
}

void Worker::respond(client_state& cs, httpRequest& request, int client_fd) {
    httpResponse res{this->router.exec_handler(request)};
    message_status send_status = cs.socket.send_message(res.finish_res());
    if (send_status == message_status::BLOCKED) {
        cs.phase = request_phase::RESPONSE;
        struct kevent change;
        EV_SET(&change, client_fd, EVFILT_WRITE, EV_ADD, 0, 0, nullptr);
        kevent(this->kq_fd, &change, 1, nullptr, 0, nullptr);
    } else {
        this->remove_client(client_fd);
    }
}

void Worker::handle_status(client_state& cs, message_status stat, int client_fd) {
    if (stat == message_status::CLOSED) {
        this->remove_client(client_fd);
        return;
    }
    if (stat == message_status::BLOCKED) {
        return;
    }

    // stat == message_status::COMPLETE
    if (cs.phase == request_phase::READHEADER) {
        std::string req_msg = cs.socket.extract_message();
        string_dict request_map{extract_string(req_msg)};

        if (request_map.empty()) { this->remove_client(client_fd); return; }

        httpRequest request{request_map["method"], request_map["path"], request_map["version"]};
        request.fill_headers(request_map);

        if (request.get_method() == "POST") {
            std::string content_length_str{get_header_ci(request.get_header(), "Content-Length")};
            int content_length = 0;
            try {
                content_length = std::stoi(content_length_str);
            } catch (const std::exception&) {
                content_length = 0;
            }

            if (content_length > 0) {
                cs.content_length = content_length;
                cs.phase = request_phase::READBODY;
                cs.request = std::move(request);

                message_status body_status = cs.socket.get_message(cs.content_length);
                if (body_status == message_status::COMPLETE) {
                    cs.request->set_body(cs.socket.extract_message(cs.content_length));
                    this->respond(cs, *cs.request, client_fd);
                } else if (body_status == message_status::CLOSED) {
                    this->remove_client(client_fd);
                }
                return;
            }
        }

        this->respond(cs, request, client_fd);
        return;
    }

    if (cs.phase == request_phase::READBODY) {
        cs.request->set_body(cs.socket.extract_message(cs.content_length));
        this->respond(cs, *cs.request, client_fd);
    }
}

Worker::~Worker() {
    this->should_stop = true;
    if (this->t_worker.joinable()) {
        char byte = 'X';
        write(this->alert.write, &byte, 1);
        this->t_worker.join();
    }
    close(this->alert.read);
    close(this->alert.write);
    if (this->kq_fd >= 0) {
        close(this->kq_fd);
    }
}
