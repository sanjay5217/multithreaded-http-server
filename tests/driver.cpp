#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <chrono>
#include <algorithm>
#include <vector>

#include "../includes/utils.hpp"
#include "../src/utils.cpp"
#include "../includes/socket.hpp"
#include "../src/socket.cpp"
#include "../includes/request.hpp"
#include "../src/request.cpp"
#include "../includes/response.hpp"
#include "../src/response.cpp"
#include "../includes/handles.hpp"
#include "../src/handles.cpp"
#include "../includes/router.hpp"
#include "../src/router.cpp"
#include "../includes/thread-pool.hpp"
#include "../src/thread-pool.cpp"

static std::string escape(const std::string& s);

//HELPERS
static void cmd_handles(const std::string& method, const std::string& path) {
    std::string body{std::istreambuf_iterator<char>(std::cin), {}};

    httpRequest req{method, path, "HTTP/1.1"};
    if (!body.empty()) {
        req.set_body(body);
    }

    Router router{};
    httpResponse res{router.exec_handler(req)};
    std::string full{res.finish_res()};

    auto crlf{full.find("\r\n")};
    std::string status_line{(crlf != std::string::npos) ? full.substr(0, crlf) : full};

    std::string resp_body{};
    auto body_start = full.find("\r\n\r\n");
    if (body_start != std::string::npos) {
        resp_body = full.substr(body_start + 4);
        while (!resp_body.empty() && (resp_body.back() == '\r' || resp_body.back() == '\n')) {
            resp_body.pop_back();
        }
    }

    std::cout << "status\t" << status_line        << "\n";
    std::cout << "body\t"   << escape(resp_body) << "\n";
}


// Run multiple METHOD\tPATH\tBODY lines through one Router in sequence.
// Output: N:status\t... and N:body\t... per request.
static void cmd_handles_seq() {
    std::string input{std::istreambuf_iterator<char>(std::cin), {}};
    Router router{};
    int idx{0};
    std::istringstream ss{input};
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty()) continue;

        auto tab1{line.find('\t')};
        if (tab1 == std::string::npos) continue;
        std::string method{line.substr(0, tab1)};
        std::string rest{line.substr(tab1 + 1)};

        std::string path, body;
        auto tab2{rest.find('\t')};
        if (tab2 != std::string::npos) {
            path = rest.substr(0, tab2);
            body = rest.substr(tab2 + 1);
        } else {
            path = rest;
        }

        httpRequest req{method, path, "HTTP/1.1"};
        if (!body.empty()) req.set_body(body);

        httpResponse res{router.exec_handler(req)};
        std::string full{res.finish_res()};

        auto crlf{full.find("\r\n")};
        std::string status_line{(crlf != std::string::npos) ? full.substr(0, crlf) : full};

        std::string resp_body{};
        auto body_start{full.find("\r\n\r\n")};
        if (body_start != std::string::npos) {
            resp_body = full.substr(body_start + 4);
            while (!resp_body.empty() && (resp_body.back() == '\r' || resp_body.back() == '\n'))
                resp_body.pop_back();
        }

        std::string prefix{std::to_string(idx) + ":"};
        std::cout << prefix << "status\t" << status_line       << "\n";
        std::cout << prefix << "body\t"   << escape(resp_body) << "\n";
        ++idx;
    }
}

static void cmd_utils() {
    std::string msg{std::istreambuf_iterator<char>(std::cin), {}};
    string_dict result{extract_string(msg)};
    for (const auto& [k, v] : result) {
        std::cout << k << "\t" << v << "\n";
    }
}

// Escape \r and \n so the value fits on a single output line.
static std::string escape(const std::string& s) {
    std::string out{};
    out.reserve(s.size());
    for (char c : s) {
        if      (c == '\r') out += "\\r";
        else if (c == '\n') out += "\\n";
        else                out += c;
    }
    return out;
}

//SOCKETS

// Create a connected socketpair. Exits on failure.
static void make_pair(int fds[2]) {
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
        std::cerr << "socketpair failed\n";
        std::exit(1);
    }
}

// send_recv: send stdin through Socket::send_message, receive with get_message().
// Prints: send_ok\t<true|false>
//         message\t<received text>
static void cmd_socket_send_recv() {
    std::string msg{std::istreambuf_iterator<char>(std::cin), {}};
    int fds[2];
    make_pair(fds);

    bool ok{};
    {
        Socket sender{fds[0]};
        ok = sender.send_message(msg);
        // Destructor closes fds[0], signaling EOF to the receiver.
    }

    Socket receiver{fds[1]};
    std::string received{receiver.get_message()};

    std::cout << "send_ok\t" << (ok ? "true" : "false") << "\n";
    std::cout << "message\t" << escape(received) << "\n";
}

// no_term: send stdin without \r\n\r\n, close write end, get_message() must return "".
// Prints: message\t<received text>
static void cmd_socket_no_term() {
    std::string msg{std::istreambuf_iterator<char>(std::cin), {}};
    int fds[2];
    make_pair(fds);

    ::send(fds[0], msg.c_str(), msg.size(), 0);
    ::shutdown(fds[0], SHUT_WR);
    ::close(fds[0]);

    Socket receiver{fds[1]};
    std::string received{receiver.get_message()};
    std::cout << "message\t" << escape(received) << "\n";
}

// send_recv_length <n>: send stdin, receive with get_message(n).
static void cmd_socket_send_length(int length) {
    std::string msg{std::istreambuf_iterator<char>(std::cin), {}};
    bool ok{};
    int fds[2];
    make_pair(fds);
    {
        Socket sender{fds[0]};
        ok = sender.send_message(msg);
        // Destructor closes fds[0], signaling EOF to the receiver.
    }

    Socket receiver{fds[1]};
    std::string received{receiver.get_message(length)};

    std::cout << "send_ok\t" << (ok ? "true" : "false") << "\n";
    std::cout << "message\t" << escape(received) << "\n";
}


//THREADPOOL

// threadpool <num_workers> <num_clients> <delay_ms>:
// Pushes num_clients real (socketpair) connections into a real ThreadPool.
// Each fake client waits delay_ms before sending its request, so if the pool
// truly runs jobs in parallel, every client should finish around delay_ms --
// not delay_ms stacked num_clients times over, which is what serialized
// handling would look like.
// Prints: total_ms, max_complete_ms, all_ok, num_workers, num_clients, delay_ms
static void cmd_threadpool_concurrency(int num_workers, int num_clients, int delay_ms) {
    Router router{};
    ThreadPool pool{num_workers, router};

    std::vector<int> server_fds(num_clients);
    std::vector<int> client_fds(num_clients);
    for (int i = 0; i < num_clients; i++) {
        int fds[2];
        make_pair(fds);
        server_fds[i] = fds[0];
        client_fds[i] = fds[1];
    }

    for (int i = 0; i < num_clients; i++) {
        pool.push(server_fds[i]);
    }

    auto overall_start{std::chrono::steady_clock::now()};
    std::vector<double> complete_ms(num_clients, -1.0);
    std::vector<bool> ok(num_clients, false);

    std::vector<std::thread> fake_clients;
    for (int i = 0; i < num_clients; i++) {
        fake_clients.emplace_back([&, i]() {
            if (delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
            std::string req{"GET /health HTTP/1.1\r\nHost: x\r\n\r\n"};
            ::send(client_fds[i], req.c_str(), req.size(), 0);

            char buf[4096];
            ssize_t n{::recv(client_fds[i], buf, sizeof(buf), 0)};

            complete_ms[i] = std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - overall_start).count();
            ok[i] = (n > 0) && (std::string(buf, n).find("200 OK") != std::string::npos);
            ::close(client_fds[i]);
        });
    }
    for (auto& t : fake_clients) t.join();

    double total_ms{std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - overall_start).count()};
    double max_complete_ms{*std::max_element(complete_ms.begin(), complete_ms.end())};
    bool all_ok{std::all_of(ok.begin(), ok.end(), [](bool v) { return v; })};

    std::cout << "total_ms\t"        << total_ms        << "\n";
    std::cout << "max_complete_ms\t" << max_complete_ms << "\n";
    std::cout << "all_ok\t"          << (all_ok ? "true" : "false") << "\n";
    std::cout << "num_workers\t"     << num_workers      << "\n";
    std::cout << "num_clients\t"     << num_clients      << "\n";
    std::cout << "delay_ms\t"        << delay_ms         << "\n";
}


// DISPATCH

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: driver <utils|socket <subcommand>>\n";
        return 1;
    }

    std::string cmd{argv[1]};

    if (cmd == "handles") {
        if (argc < 4) {
            std::cerr << "Usage: driver handles <method> <path>\n";
            return 1;
        }
        cmd_handles(argv[2], argv[3]);
        return 0;
    }

    if (cmd == "handles_seq") {
        cmd_handles_seq();
        return 0;
    }

    if (cmd == "utils") {
        cmd_utils();
        return 0;
    }

    if (cmd == "socket") {
        if (argc < 3) {
            std::cerr << "Usage: driver socket <send_recv|no_term>\n";
            return 1;
        }
        std::string subcmd{argv[2]};
        if (subcmd == "send_recv") {
            cmd_socket_send_recv();
        } else if (subcmd == "send_recv_length") {
            int length{(argc >= 4) ? std::atoi(argv[3]) : 5};
            cmd_socket_send_length(length);
        } else if (subcmd == "no_term") {
            cmd_socket_no_term();
        } else {
            std::cerr << "Unknown socket subcommand: " << subcmd << "\n";
            return 1;
        }
        return 0;
    }

    if (cmd == "threadpool") {
        if (argc < 5) {
            std::cerr << "Usage: driver threadpool <num_workers> <num_clients> <delay_ms>\n";
            return 1;
        }
        cmd_threadpool_concurrency(std::atoi(argv[2]), std::atoi(argv[3]), std::atoi(argv[4]));
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    return 1;
}
