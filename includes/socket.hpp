#pragma once 

#include <iostream>
#include <cstring>
#include <string>
#include <unordered_map>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

const int CONST_BUFFER_SIZE = 4096;

class Socket {
    private:
        int fd;
        std::string buffer;
        size_t buffer_size;

    public:
        /**
        * @brief Constructs a socket.
        *
        * @param socket_fd File descriptor for the socket.
        */
        Socket(int socket_fd);

        /**
        * @brief Reads all available data from the socket into a string.
        *
        * @return The received message.
        */
        std::string get_message();

        /**
        * @brief Reads exactly @param length bytes from the socket.
        *
        * @param length Number of bytes to read.
        * @return The received message.
        */
        std::string get_message(int length);

        /**
        * @brief Sends a message through the socket.
        *
        * @param msg Message to send.
        * @return True if successful, false otherwise.
        */
        bool send_message(const std::string& msg);

        /**
        * @brief Closes the socket fd.
        */
        ~Socket();
};
