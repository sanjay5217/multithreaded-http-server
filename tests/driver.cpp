#include <iostream>
#include <iterator>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "../src/utils.hpp"
#include "../src/utils.cpp"
#include "../src/socket.hpp"
#include "../src/socket.cpp"

//HELPERS 

static void cmd_utils() {
    std::string msg(std::istreambuf_iterator<char>(std::cin), {});
    string_dict result = extract_string(msg);
    for (const auto& [k, v] : result) {
        std::cout << k << "\t" << v << "\n";
    }
}

// Escape \r and \n so the value fits on a single output line.
static std::string escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if      (c == '\r') out += "\\r";
        else if (c == '\n') out += "\\n";
        else                out += c;
    }
    return out;
}

// ---------------------------------------------------------------------------
// socket subcommands
// ---------------------------------------------------------------------------

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
    std::string msg(std::istreambuf_iterator<char>(std::cin), {});
    int fds[2];
    make_pair(fds);

    bool ok;
    {
        Socket sender(fds[0]);
        ok = sender.send_message(msg);
        // Destructor closes fds[0], signaling EOF to the receiver.
    }

    Socket receiver(fds[1]);
    std::string received = receiver.get_message();

    std::cout << "send_ok\t" << (ok ? "true" : "false") << "\n";
    std::cout << "message\t" << escape(received) << "\n";
}

// no_term: send stdin without \r\n\r\n, close write end, get_message() must return "".
// Prints: message\t<received text>
static void cmd_socket_no_term() {
    std::string msg(std::istreambuf_iterator<char>(std::cin), {});
    int fds[2];
    make_pair(fds);

    ::send(fds[0], msg.c_str(), msg.size(), 0);
    ::shutdown(fds[0], SHUT_WR);
    ::close(fds[0]);

    Socket receiver(fds[1]);
    std::string received = receiver.get_message();
    std::cout << "message\t" << escape(received) << "\n";
}

// send_recv_length <n>: send stdin, receive with get_message(n).
static void cmd_socket_send_length(int length) {
    std::string msg(std::istreambuf_iterator<char>(std::cin), {});
    bool ok;
    int fds[2];
    make_pair(fds);
    {
        Socket sender(fds[0]);
        ok = sender.send_message(msg);
        // Destructor closes fds[0], signaling EOF to the receiver.
    }

    Socket receiver(fds[1]);
    std::string received = receiver.get_message(length);

    std::cout << "send_ok\t" << (ok ? "true" : "false") << "\n";
    std::cout << "message\t" << escape(received) << "\n";
}


// ---------------------------------------------------------------------------
// Dispatch
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: driver <utils|socket <subcommand>>\n";
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "utils") {
        cmd_utils();
        return 0;
    }

    if (cmd == "socket") {
        if (argc < 3) {
            std::cerr << "Usage: driver socket <send_recv|no_term>\n";
            return 1;
        }
        std::string subcmd = argv[2];
        if (subcmd == "send_recv") {
            cmd_socket_send_recv();
        } else if (subcmd == "send_recv_length") {
            int length = (argc >= 4) ? std::atoi(argv[3]) : 5;
            cmd_socket_send_length(length);
        } else if (subcmd == "no_term") {
            cmd_socket_no_term();
        } else {
            std::cerr << "Unknown socket subcommand: " << subcmd << "\n";
            return 1;
        }
        return 0;
    }

    std::cerr << "Unknown command: " << cmd << "\n";
    return 1;
}
