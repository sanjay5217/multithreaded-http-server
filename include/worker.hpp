#pragma once

#include <sys/event.h>
#include <array>
#include <thread>
#include <queue>
#include <atomic>
#include <algorithm>
#include <optional>
#include <mutex>

#include "utils.hpp"
#include "router.hpp"
#include "request.hpp"
#include "response.hpp"
#include "socket.hpp"

struct alert_thread {
    int read;
    int write;
};

enum class request_phase {
    READHEADER,
    READBODY,
    RESPONSE
};

struct client_state {
    Socket socket;
    request_phase phase;
    int content_length;
    std::optional<httpRequest> request;
};

using socket_map = std::unordered_map<int, client_state>;


class Worker {
    public:
        std::thread t_worker;

        Worker(Router& router, std::function<void()> slot_freed);
        Worker(const Worker& other) = delete;
        Worker& operator=(const Worker& other) = delete;
        Worker(Worker&& other);
        Worker& operator=(Worker&& other) = delete;
        ~Worker();

        /**
         * @brief Accepts the client
         * @param client_fd Client file descriptor
         */
        void accept_client(int client_fd);

        /**
         * @brief Queues a client for this worker to pick up
         *
         * @param client_fd Client file descriptor
         */
        void enqueue_client(int client_fd);

        /**
         * @brief Assigns the thread work
         */
        void assign_work(void);

        /**
         * @return True if there is space for a client and false otherwise
         */
        bool reserve_slot(void);

        /**
         * @return The pipe's write file descriptor
         */
        int get_write_fd(void);

        /**
         * @return Worker's client capacity
         */
        int get_capacity(void);

        /**
         * @return Current client occupancy
         */
        int get_occupancy(void);

    private:
        Router& router;
        const int capacity;
        static constexpr int CLIENT_NUMBER = 10;
        std::atomic <int> current;
        int kq_fd;
        struct alert_thread alert;
        socket_map clients;
        std::function<void()> on_slot_freed;
        std::atomic<bool> should_stop{false};

        // Bug (found via concurrent stress testing, ASan
        // bad-__sanitizer_annotate_double_ended_contiguous_container abort
        // inside std::deque::pop_front): client_queue used to be a plain,
        // unsynchronized std::queue<int> pushed to from the ThreadPool's
        // accept-loop thread and popped from this worker's own thread with
        // no lock at all, corrupting the underlying deque under load. Fix:
        // keep it private and guard every access with queue_mutex via
        // enqueue_client() / the locked pop in handle_client().
        std::queue<int> client_queue;
        std::mutex queue_mutex;

        /**
         * @brief Removes the client
         * @param client_fd Client file descriptor
         */
        void remove_client(int client_fd);

        /**
         * @brief Worker thread's main loop. Registers new clients and handles
         *        existing ones by dispatching each to the appropriate handler.
         */
        void handle_client(void);

        /**
         * @brief Advances a client's request based on the result of the last
         *        socket read. Handles connection close/blocked reads, parses
         *        completed headers, transitions into reading the body for
         *        POST requests with a Content-Length, and triggers a response
         *        once the request is fully read.
         *
         * @param cs The client's state
         * @param stat Status of the last read from the client's socket
         * @param client_fd Client file descriptor
         */
        void handle_status(client_state& cs, message_status stat, int client_fd);

        /**
         * @brief Executes the matched route handler and sends the response
         *        to the client if response is complete. Otherwise leaves 
         *        the client in response phase to be resumed later. Removes
         *        the client if response is sent.
         *
         * @param cs The client's state
         * @param request The fully-read HTTP request to handle
         * @param client_fd Client file descriptor
         */
        void respond(client_state& cs, httpRequest& request, int client_fd);
};
    
      