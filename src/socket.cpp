#include "../includes/socket.hpp"


Socket::Socket(int socket_fd) {
    this->fd = socket_fd;
    this->buffer = "";
    this->buffer_size = 0;
}

std::string Socket::get_message(void) {
    char temp[CONST_BUFFER_SIZE];
    std::string msg;
    ssize_t bytes_received;

    while ((bytes_received = recv(this->fd, temp, sizeof(temp) - 1, 0)) > 0) {
        this->buffer.append(temp, bytes_received);
        this->buffer_size+=bytes_received;

        size_t pos = this->buffer.find("\r\n\r\n");
        if (pos != std::string::npos) {
            msg = this->buffer.substr(0, pos + 4);
            this->buffer.erase(0, pos + 4);
            return msg;
        }
    }
    
    return "";
}

std::string Socket::get_message(int length) {
    std::string msg;

    if (this->buffer_size >= length) {
        msg = buffer.substr(0, length);
        buffer.erase(0, length);
        return msg;
    }

    msg = buffer;
    buffer.clear();
    char temp[CONST_BUFFER_SIZE];
    ssize_t bytes_received;

    while (msg.size() < (size_t)length &&
                (bytes_received = recv(this->fd, temp, sizeof(temp) - 1, 0)) > 0) {
        size_t amount = length - msg.size();
        size_t required = std::min(amount, (size_t)bytes_received);
        msg.append(temp, required);
        if ((size_t)bytes_received > required) {
            this->buffer.append(temp + required, bytes_received - required);
        }
    }
    return msg;
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
