#include "../includes/utils.hpp"
#include "../includes/socket.hpp"
#include "../includes/response.hpp"
#include "../includes/request.hpp"
#include "../includes/router.hpp"

// #include <thread> // Required for std::this_thread::sleep_for
// #include <chrono> // Required for std::chrono::seconds

// Constants
const int PORT = 8080;

// Functions 

int main() {
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

    // Wrap into socket class
    Socket server_socket{server_fd};

    Router router{};
    while(1) {
        sockaddr_in client_addr{};
        socklen_t client_len{sizeof(client_addr)};
        int client_fd{};
        
        if ((client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len)) < 0) {
            perror("accept");
            return 1;
        };

        Socket client_socket{client_fd};
        
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        std::cout << "Connected from "
                    << client_ip
                    << ":"
                    << ntohs(client_addr.sin_port)
                    << '\n'
                    << std::endl;

        std::string req_msg{};
        req_msg = client_socket.get_message();
        string_dict request_map{extract_string(req_msg)};
        if (request_map.empty()) { return 1; }
        httpRequest request{request_map["method"], request_map["path"], request_map["version"]};
        request.fill_headers(request_map);

        if (request.get_method() == "POST" && request.get_path() != "/compute") {
            string_dict map{request.get_header()};
            std::cout << map["Content-Length"] << std::endl;
            int content_length{std::stoi(request.get_header()["Content-Length"])};
            request.set_body(client_socket.get_message(content_length));
            std::cout << request.get_body() << "hi" << std::endl;
        }

        std::cout << req_msg << std::endl;
        httpRequest &r_req = request;

        httpResponse res{router.exec_handler(r_req)};
        client_socket.send_message(res.finish_res());
    };

    return 0;
}