#include "../include/utils.hpp"
#include "../include/socket.hpp"
#include "../include/response.hpp"
#include "../include/request.hpp"
#include "../include/router.hpp"
#include "../include/thread-pool.hpp"
#include <csignal>

// Constants
const int PORT = 8080;
const int THREAD_POOL_NUMBER = 10;

int main() {
    signal(SIGPIPE, SIG_IGN);

    // Create socket
    int server_fd{socket(AF_INET, SOCK_STREAM, 0)};
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // Allow port reuse after restart
    int opt{1};
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Server address
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind
    if (bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    // Listen
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "Listening on port " << PORT << " ...\n";


    Router router{};
    ThreadPool thread_pool{THREAD_POOL_NUMBER, std::ref(router)};

    while(1) {
        sockaddr_in client_addr{};
        socklen_t client_len{sizeof(client_addr)};
        int client_fd{};
        
        if ((client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len)) < 0) {
            perror("accept");
            return 1;
        };

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        std::cout << "Connected from "
                    << client_ip
                    << ":"
                    << ntohs(client_addr.sin_port)
                    << '\n'
                    << std::endl;

        thread_pool.push(client_fd);
    };

    return 0;
}
