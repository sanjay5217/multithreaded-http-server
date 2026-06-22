#pragma once 

#include <iostream>
#include <cstring>
#include <string>
#include <unordered_map>
#include <optional>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

const int CONST_BUFFER_SIZE = 4096;
enum class message_status {COMPLETE, BLOCKED, CLOSED};

class Socket {
    private:
        int fd;
        std::string buffer;
        size_t buffer_size;
        std::string send_buffer;
        size_t send_offset;

    public:
        message_status status;

        Socket(int socket_fd);
        Socket(const Socket& other) = delete;
        Socket& operator=(const Socket& other) = delete;
        Socket(Socket&& other);
        Socket& operator=(Socket&& other);
        ~Socket();

        /**
        * @brief Stores the HTTP body message sent through the socket in a buffer
        *
        * @param length The amount to read from
        * @return COMPLETE if the whole message is stored, BLOCKED if the socket
        *         would block, CLOSED on error/disconnect.
        */
        message_status get_message(int length);

        /**
        * @brief Stores the HTTP message sent through the socket in a buffer
        *
        * @return COMPLETE if the whole HTTP message is stored, BLOCKED if the socket
        *         would block, CLOSED on error/disconnect.
        */       
        message_status get_message(void);

        /**
        * @brief Flushes the message stored internally in the socket
        *
        * @return The string format of the message
        */         
        std::string extract_message(std::optional<int> length = std::nullopt);

        /**
        * @brief Sends a message through the socket, resuming a partial send
        *        from where the previous call left off.
        *
        * @param msg Message to send. Ignored if a send is already in progress.
        * @return COMPLETE if the whole message went out, BLOCKED if the socket
        *         would block, CLOSED on error/disconnect.
        */
        message_status send_message(const std::string& msg);
};
