#include <string>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "socket.hpp"

/*
 * Constructor for Socket class
*/
Socket::Socket(int socket_fd) {
    this->fd = socket_fd;
    this->buffer = "";
}

/*
 * retrieves message written to <fd> and returns it as a string
 * returns empty string if no message exists
*/
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

/*
 * sends message to <fd> and returns true if successful and false otherwise
*/
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

/*
 * Deconstructor
*/
Socket::~Socket() {
    close(this->fd);
}
