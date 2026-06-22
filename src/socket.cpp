#include "../include/socket.hpp"

Socket::Socket(int socket_fd) :
    fd{socket_fd}, buffer{}, buffer_size{0}, send_buffer{}, send_offset{0} {
    fcntl(this->fd, F_SETFL, O_NONBLOCK);
}

Socket::Socket(Socket&& other) :
    fd{other.fd}, buffer{std::move(other.buffer)}, buffer_size{other.buffer_size},
    send_buffer{std::move(other.send_buffer)}, send_offset{other.send_offset} {
        other.fd = -1;
    }

Socket& Socket::operator=(Socket&& other) {
    if (this != &other) {
        close(this->fd);
        this->fd = other.fd;
        this->buffer = std::move(other.buffer);
        this->buffer_size = other.buffer_size;
        this->send_buffer = std::move(other.send_buffer);
        this->send_offset = other.send_offset;
        other.fd = -1;
    }
    return *this;
}

message_status Socket::get_message() {
    char temp[CONST_BUFFER_SIZE];
    ssize_t bytes_received;

    while ((bytes_received = recv(this->fd, temp, sizeof(temp) - 1, 0)) >= 0) {
        if (bytes_received == 0) {
            return message_status::CLOSED;
        }

        this->buffer.append(temp, bytes_received);
        this->buffer_size+=bytes_received;

        size_t pos = this->buffer.find("\r\n\r\n");
        if (pos != std::string::npos) {
            return message_status::COMPLETE;
        }
    }

    if (errno == EAGAIN) {return message_status::BLOCKED;}
    else {return message_status::CLOSED;}
}

message_status Socket::get_message(int length) {
    if (this->buffer_size >= (size_t)length) { return message_status::COMPLETE; }

    char temp[CONST_BUFFER_SIZE];
    ssize_t bytes_received;

    while (this->buffer_size < (size_t)length &&
                (bytes_received = recv(this->fd, temp, sizeof(temp) - 1, 0)) > 0) {

        size_t amount = length - this->buffer_size;
        size_t required = std::min(amount, (size_t)bytes_received);
        this->buffer.append(temp, required);
        this->buffer_size+=required;
    }

    if (this->buffer_size >= (size_t)length) { return message_status::COMPLETE; }
    if (bytes_received == 0) { return message_status::CLOSED; }
    if (bytes_received == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) { return message_status::BLOCKED; }
        return message_status::CLOSED;
    }
    return message_status::CLOSED;
}

std::string Socket::extract_message(std::optional<int> length) {
    size_t pos;
    if (!length.has_value()) {
        pos = this->buffer.find("\r\n\r\n");
        pos += 4;
    } else { pos = static_cast<size_t>(*length); }
    std::string msg = this->buffer.substr(0, pos);
    this->buffer.erase(0, pos);
    this->buffer_size-=pos;
    return msg;
}

message_status Socket::send_message(const std::string& msg) {
    if (this->send_offset == 0 && this->send_buffer.empty()) {
        this->send_buffer = msg;
    }

    while (this->send_offset < this->send_buffer.size()) {
        ssize_t bytes_sent = send(this->fd, this->send_buffer.c_str() + this->send_offset,
                                   this->send_buffer.size() - this->send_offset, 0);
        if (bytes_sent == 0) { return message_status::CLOSED; }
        if (bytes_sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) { return message_status::BLOCKED; }
            return message_status::CLOSED;
        }
        this->send_offset += bytes_sent;
    }
    this->send_buffer.clear();
    this->send_offset = 0;
    return message_status::COMPLETE;
}

Socket::~Socket() {
    if (this->fd >= 0) {
        close(this->fd);
    }
}
