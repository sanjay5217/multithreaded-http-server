#include "socket.hpp"


Socket::Socket(int socket_fd) {
    this->fd = socket_fd;
    this->buffer = "";
}

std::string Socket::get_message(void) {
    char temp[CONST_BUFFER_SIZE];
    std::string msg;
    ssize_t bytes_received;

    while ((bytes_received = recv(this->fd, temp, sizeof(temp) - 1, 0)) > 0) {
        this->buffer.append(temp, bytes_received);

        size_t pos = this->buffer.find("\r\n\r\n");
        if (pos != std::string::npos) {
            msg = this->buffer.substr(0, pos + 4);
            this->buffer.erase(0, pos + 4);
            return msg;
        }
    }
    return "";
}

bool Socket::send_message(std::string msg) {
    ssize_t bytes_sent;
    size_t count = 0;

    while (count < msg.size()) {
        bytes_sent = send(this->fd, msg.c_str() + count, msg.size() - count, 0);
        if (bytes_sent < 0) { return false; }
        count += bytes_sent;
    }

    return true;
}

Socket::~Socket() {
    close(this->fd);
}
